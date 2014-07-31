/*
 * Copyright (C) 2007-2008 Advanced Micro Devices, Inc.
 * Author: Joerg Roedel <joerg.roedel@amd.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#define pr_fmt(fmt)    "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/bug.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/iommu.h>
#include <linux/scatterlist.h>

static struct iommu_ops *iommu_ops;

void register_iommu(struct iommu_ops *ops)
{
	if (iommu_ops)
		BUG();

	iommu_ops = ops;
}

bool iommu_found(void)
{
	return iommu_ops != NULL;
}
EXPORT_SYMBOL_GPL(iommu_found);

	/*
	 * Set the default pgsize values, which retain the existing
	 * IOMMU API behavior: drivers will be called to map
	 * regions that are sized/aligned to order of 4KiB pages.
	 *
	 * This will be removed once all drivers are migrated.
	 */
	if (!ops->pgsize_bitmap)
		ops->pgsize_bitmap = ~0xFFFUL;

/**
 * iommu_set_fault_handler() - set a fault handler for an iommu domain
 * @domain: iommu domain
 * @handler: fault handler
 *
 * This function should be used by IOMMU users which want to be notified
 * whenever an IOMMU fault happens.
 *
 * The fault handler itself should return 0 on success, and an appropriate
 * error code otherwise.
 */
void iommu_set_fault_handler(struct iommu_domain *domain,
					iommu_fault_handler_t handler)
{
	BUG_ON(!domain);

	domain->handler = handler;
}
EXPORT_SYMBOL_GPL(iommu_set_fault_handler);

struct iommu_domain *iommu_domain_alloc(int flags)
{
	struct iommu_domain *domain;
	int ret;

	if (!iommu_found())
		return NULL;

	domain = kmalloc(sizeof(*domain), GFP_KERNEL);
	if (!domain)
		return NULL;

	ret = iommu_ops->domain_init(domain, flags);
	if (ret)
		goto out_free;

	return domain;

out_free:
	kfree(domain);

	return NULL;
}
EXPORT_SYMBOL_GPL(iommu_domain_alloc);

void iommu_domain_free(struct iommu_domain *domain)
{
	if (!iommu_found())
		return;

	iommu_ops->domain_destroy(domain);
	kfree(domain);
}
EXPORT_SYMBOL_GPL(iommu_domain_free);

int iommu_attach_device(struct iommu_domain *domain, struct device *dev)
{
	if (!iommu_found())
		return -ENODEV;

	return iommu_ops->attach_dev(domain, dev);
}
EXPORT_SYMBOL_GPL(iommu_attach_device);

void iommu_detach_device(struct iommu_domain *domain, struct device *dev)
{
	if (!iommu_found())
		return;

	iommu_ops->detach_dev(domain, dev);
}
EXPORT_SYMBOL_GPL(iommu_detach_device);

phys_addr_t iommu_iova_to_phys(struct iommu_domain *domain,
			       unsigned long iova)
{
	if (!iommu_found())
		return 0;

	return iommu_ops->iova_to_phys(domain, iova);
}
EXPORT_SYMBOL_GPL(iommu_iova_to_phys);

int iommu_domain_has_cap(struct iommu_domain *domain,
			 unsigned long cap)
{
	if (!iommu_found())
		return -ENODEV;

	return iommu_ops->domain_has_cap(domain, cap);
}
EXPORT_SYMBOL_GPL(iommu_domain_has_cap);

int iommu_map(struct iommu_domain *domain, unsigned long iova,
	      phys_addr_t paddr, size_t size, int prot)
{
	unsigned long orig_iova = iova;
	unsigned int min_pagesz;
	size_t orig_size = size;
	int ret = 0;

	if (!iommu_found())
		return -ENODEV;

	/* find out the minimum page size supported */
	min_pagesz = 1 << __ffs(domain->ops->pgsize_bitmap);

	/*
	 * both the virtual address and the physical one, as well as
	 * the size of the mapping, must be aligned (at least) to the
	 * size of the smallest page supported by the hardware
	 */
	if (!IS_ALIGNED(iova | paddr | size, min_pagesz)) {
		pr_err("unaligned: iova 0x%lx pa 0x%lx size 0x%lx min_pagesz "
			"0x%x\n", iova, (unsigned long)paddr,
			(unsigned long)size, min_pagesz);
		return -EINVAL;
	}

	pr_debug("map: iova 0x%lx pa 0x%lx size 0x%lx\n", iova,
				(unsigned long)paddr, (unsigned long)size);

	while (size) {
		unsigned long pgsize, addr_merge = iova | paddr;
		unsigned int pgsize_idx;

		/* Max page size that still fits into 'size' */
		pgsize_idx = __fls(size);

		/* need to consider alignment requirements ? */
		if (likely(addr_merge)) {
			/* Max page size allowed by both iova and paddr */
			unsigned int align_pgsize_idx = __ffs(addr_merge);

			pgsize_idx = min(pgsize_idx, align_pgsize_idx);
		}

		/* build a mask of acceptable page sizes */
		pgsize = (1UL << (pgsize_idx + 1)) - 1;

		/* throw away page sizes not supported by the hardware */
		pgsize &= domain->ops->pgsize_bitmap;

		/* make sure we're still sane */
		BUG_ON(!pgsize);

		/* pick the biggest page */
		pgsize_idx = __fls(pgsize);
		pgsize = 1UL << pgsize_idx;

		pr_debug("mapping: iova 0x%lx pa 0x%lx pgsize %lu\n", iova,
					(unsigned long)paddr, pgsize);

		ret = domain->ops->map(domain, iova, paddr, pgsize, prot);
		if (ret)
			break;

		iova += pgsize;
		paddr += pgsize;
		size -= pgsize;
	}

	/* unroll mapping in case something went wrong */
	if (ret)
		iommu_unmap(domain, orig_iova, orig_size - size);

	return ret;
}
EXPORT_SYMBOL_GPL(iommu_map);

size_t iommu_unmap(struct iommu_domain *domain, unsigned long iova, size_t size)
{
	size_t unmapped_page, unmapped = 0;
	unsigned int min_pagesz;

	if (!iommu_found())
		return -ENODEV;

	/* find out the minimum page size supported */
	min_pagesz = 1 << __ffs(domain->ops->pgsize_bitmap);

	/*
	 * The virtual address, as well as the size of the mapping, must be
	 * aligned (at least) to the size of the smallest page supported
	 * by the hardware
	 */
	if (!IS_ALIGNED(iova | size, min_pagesz)) {
		pr_err("unaligned: iova 0x%lx size 0x%lx min_pagesz 0x%x\n",
					iova, (unsigned long)size, min_pagesz);
		return -EINVAL;
	}

	pr_debug("unmap this: iova 0x%lx size 0x%lx\n", iova,
							(unsigned long)size);

	/*
	 * Keep iterating until we either unmap 'size' bytes (or more)
	 * or we hit an area that isn't mapped.
	 */
	while (unmapped < size) {
		size_t left = size - unmapped;

		unmapped_page = domain->ops->unmap(domain, iova, left);
		if (!unmapped_page)
			break;

		pr_debug("unmapped: iova 0x%lx size %lx\n", iova,
					(unsigned long)unmapped_page);

		iova += unmapped_page;
		unmapped += unmapped_page;
	}

	return unmapped;
}
EXPORT_SYMBOL_GPL(iommu_unmap);

int iommu_map_range(struct iommu_domain *domain, unsigned int iova,
		    struct scatterlist *sg, unsigned int len, int prot)
{
	if (!iommu_found())
		return -ENODEV;

	BUG_ON(iova & (~PAGE_MASK));

	return iommu_ops->map_range(domain, iova, sg, len, prot);
}
EXPORT_SYMBOL_GPL(iommu_map_range);

int iommu_unmap_range(struct iommu_domain *domain, unsigned int iova,
		      unsigned int len)
{
	if (!iommu_found())
		return -ENODEV;

	BUG_ON(iova & (~PAGE_MASK));

	return iommu_ops->unmap_range(domain, iova, len);
}
EXPORT_SYMBOL_GPL(iommu_unmap_range);

phys_addr_t iommu_get_pt_base_addr(struct iommu_domain *domain)
{
	if (!iommu_found())
		return 0;

	return iommu_ops->get_pt_base_addr(domain);
}
EXPORT_SYMBOL_GPL(iommu_get_pt_base_addr);