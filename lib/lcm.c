#include <linux/kernel.h>
#include <linux/gcd.h>
<<<<<<< HEAD
#include <linux/module.h>
=======
#include <linux/export.h>
#include <linux/lcm.h>
>>>>>>> 8bc3bcc... lib: reduce the use of module.h wherever possible

/* Lowest common multiple */
unsigned long lcm(unsigned long a, unsigned long b)
{
	if (a && b)
		return (a * b) / gcd(a, b);
	else if (b)
		return b;

	return a;
}
EXPORT_SYMBOL_GPL(lcm);
