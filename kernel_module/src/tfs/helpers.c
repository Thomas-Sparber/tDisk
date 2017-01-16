#include "helpers.h"

/**
 * find_get_pages - gang pagecache lookup
 * @mapping:	The address_space to search
 * @start:	The starting page index
 * @nr_pages:	The maximum number of pages
 * @pages:	Where the resulting pages are placed
 *
 * find_get_pages() will search for and return a group of up to
 * @nr_pages pages in the mapping.  The pages are placed at @pages.
 * find_get_pages() takes a reference against the returned pages.
 *
 * The search returns a group of mapping-contiguous pages with ascending
 * indexes.  There may be holes in the indices due to not-present pages.
 *
 * find_get_pages() returns the number of pages which were found.
 */
unsigned find_get_pages(struct address_space *mapping, pgoff_t start,
			    unsigned int nr_pages, struct page **pages)
{
	struct radix_tree_iter iter;
	void **slot;
	unsigned ret = 0;

	if (unlikely(!nr_pages))
		return 0;

	rcu_read_lock();
	radix_tree_for_each_slot(slot, &mapping->page_tree, &iter, start) {
		struct page *page;
repeat:
		page = radix_tree_deref_slot(slot);
		if (unlikely(!page))
			continue;

		if (radix_tree_exception(page)) {
			if (radix_tree_deref_retry(page)) {
				slot = radix_tree_iter_retry(&iter);
				continue;
			}
			/*
			 * A shadow entry of a recently evicted page,
			 * or a swap entry from shmem/tmpfs.  Skip
			 * over it.
			 */
			continue;
		}

		if (!page_cache_get_speculative(page))
			goto repeat;

		/* Has the page moved? */
		if (unlikely(page != *slot)) {
			put_page(page);
			goto repeat;
		}

		pages[ret] = page;
		if (++ret == nr_pages)
			break;
	}

	rcu_read_unlock();
	return ret;
}

const struct vm_operations_struct generic_file_vm_ops = {
};


/*
 * For address_spaces which do not use buffers nor write back.
 */
int __set_page_dirty_no_writeback(struct page *page)
{
	if (!PageDirty(page))
		return !TestSetPageDirty(page);
	return 0;
}

/**
 * nommu_shrink_inode_mappings - Shrink the shared mappings on an inode
 * @inode: The inode to check
 * @size: The current filesize of the inode
 * @newsize: The proposed filesize of the inode
 *
 * Check the shared mappings on an inode on behalf of a shrinking truncate to
 * make sure that that any outstanding VMAs aren't broken and then shrink the
 * vm_regions that extend that beyond so that do_mmap_pgoff() doesn't
 * automatically grant mappings that are too large.
 */
/*int nommu_shrink_inode_mappings(struct inode *inode, size_t size,
				size_t newsize)
{
	struct vm_area_struct *vma;
	struct vm_region *region;
	pgoff_t low, high;
	size_t r_size, r_top;

	low = newsize >> PAGE_SHIFT;
	high = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;

	down_write(&nommu_region_sem);
	i_mmap_lock_read(inode->i_mapping);

	//search for VMAs that fall within the dead zone
	vma_interval_tree_foreach(vma, &inode->i_mapping->i_mmap, low, high) {
		//found one - only interested if it's shared out of the page
		//cache
		if (vma->vm_flags & VM_SHARED) {
			i_mmap_unlock_read(inode->i_mapping);
			up_write(&nommu_region_sem);
			return -ETXTBSY; //not quite true, but near enough
		}
	}

	//reduce any regions that overlap the dead zone - if in existence,
	//these will be pointed to by VMAs that don't overlap the dead zone
	//we don't check for any regions that start beyond the EOF as there
	//shouldn't be any
	vma_interval_tree_foreach(vma, &inode->i_mapping->i_mmap, 0, ULONG_MAX) {
		if (!(vma->vm_flags & VM_SHARED))
			continue;

		region = vma->vm_region;
		r_size = region->vm_top - region->vm_start;
		r_top = (region->vm_pgoff << PAGE_SHIFT) + r_size;

		if (r_top > newsize) {
			region->vm_top -= r_top - newsize;
			if (region->vm_end > region->vm_top)
				region->vm_end = region->vm_top;
		}
	}

	i_mmap_unlock_read(inode->i_mapping);
	up_write(&nommu_region_sem);
	return 0;
}*/
