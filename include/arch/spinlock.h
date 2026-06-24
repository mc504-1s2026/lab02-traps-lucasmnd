#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <kernel/defs.h>
#include <kernel/types.h>
#include <kernel/trap.h>

struct spinlock {
	/* the amoswap.w instruction operates on 32-bit values and
	 * requires 4-byte alignment */
	u32 __aligned(4) lock;
};

/*
 * spin_init(): Initialize a spinlock
 * @s: spinlock
 */
static __always_inline void spin_init(struct spinlock *s)
{
	s->lock = 0;
}

/*
 * spin_trylock(): try to acquire a spinlock
 * @s: spinlock
 *
 * Return: true if the spinlock was acquired successfully, or false otherwise.
 */
static __always_inline bool spin_trylock(struct spinlock *s)
{
	u32 val;

	/* here's how this works in case you're interested:
	 * the amoswap rd, rs2, (rs1) function will atomically do
	 *
	 * - rd = M[rs1] -> store the old value in rd
	 * - swap(rs2, M[rs1]) -> swap the memory value M[rs1] with the reg value rs2
	 *
	 * the .w suffix means we're operating on 32-bit variables (commonly called a word)
	 *
	 * the .aq suffix means "acquire constraint", which tells an out-of-order CPU
	 * that it shouldn't reorder operations such that a memory access that comes
	 * after this instruction ends up happening before it
	 *
	 * the following inline asm roughly translates to the following:
	 * a1 = &s->lock
	 * amoswap.w.aq a0, 1, (a1)
	 * return a1
	 */
	__asm__ __volatile__("amoswap.w.aq %0, %2, %1" : "=r" (val), "+A" (s->lock) : "r" (1) : "memory");
	return !val;
}

/*
 * spin_lock(): acquire a spinlock
 * @s: spinlock
 *
 * This function repeatedly tries to acquire the spinlock in a loop
 * (e.g. it spins until the lock is acquired, hence the name
 * "spinlock").
 *
 * It is very important that you always release the spinlock within a
 * finite (and preferably short) amount of time, otherwise other users
 * trying to access the lock will block. If for some reason you fail to
 * release the spinlock and some other place in the code tries to acquire
 * it with spin_lock, you will be stuck (in what is called a deadlock).
 *
 * IMPORTANT: if there is any chance that this lock is acquired within
 * an interrupt context, you should use spin_lock_irq()/spin_lock_irqsave()
 * to disable interrupts while holding the lock (otherwise if an interrupt
 * happens while you're holding the lock, you will deadlock if the irq
 * handler tries to acquire it).
 */
static __always_inline void spin_lock(struct spinlock *s)
{
	while (!spin_trylock(s)) {}
}

/*
 * spin_unlock(): release a spinlock
 * @s: spinlock
 */
static __always_inline void spin_unlock(struct spinlock *s)
{
	/* atomically write zero to s->lock; notice we use the ".rl"
	 * suffix this time (which indicates a "release constraint",
	 * the opposite of ".aq" in spin_lock())*/
	__asm__ __volatile__("amoswap.w.rl zero, zero, %0" : "+A" (s->lock) : : "memory" );
}

/*
 * spin_lock_irq(): acquire a lock with interrupts disabled
 * @s: spinlock
 *
 * This function disables interrupts before doing spin_lock(); callers should
 * release the lock with spin_unlock_irq(). */
static __always_inline void spin_lock_irq(struct spinlock *s)
{
	hart_irq_disable();
	spin_lock(s);
}

/*
 * spin_unlock_irq(): release a lock and enable interrupts
 * @s: spinlock
 *
 * This is the counterpart to spin_lock_irq(). Note that this function
 * will enable interrupts unconditionally after releasing the lock; if
 * you're interested in restoring the previous irq state (enabled/
 * disabled), use spin_lock_irqsave()/spin_unlock_irqrestore() instead.
 */
static __always_inline void spin_unlock_irq(struct spinlock *s)
{
	spin_unlock(s);
	hart_irq_enable();
}

/*
 * spin_lock_irqsave(): acquire a lock with interrupts disabled and save state
 * @s: spinlock
 *
 * This function does the essentially the same as spin_lock_irq(),
 * except that it returns the irq state flags (enabled/disabled) so
 * that the caller can restore it afterwards with spin_unlock_irqrestore().
 */
static __always_inline u64 spin_lock_irqsave(struct spinlock *s)
{
	u64 flags;
	flags = hart_irq_save();
	spin_lock(s);
	return flags;
}

/*
 * spin_unlock_irqrestore(): release a lock and restore previous irq state
 * @s: spinlock
 *
 * This function releases the lock and then restores previous irq state
 * (obtained by calling spin_lock_irqsave() previously).
 */
static __always_inline void spin_unlock_irqrestore(struct spinlock *s, u64 flags)
{
	spin_unlock(s);
	hart_irq_restore(flags);
}

#endif
