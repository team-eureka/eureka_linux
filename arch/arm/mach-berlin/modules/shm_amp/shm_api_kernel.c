/********************************************************************************
 * Marvell GPL License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the General
 * Public License Version 2, June 1991 (the "GPL License"), a copy of which is
 * available along with the File in the license.txt file or by writing to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
 * on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED.  The GPL License provides additional details about this warranty
 * disclaimer.
 ********************************************************************************/


/*******************************************************************************
  System head files
*/

#include <linux/kernel.h>	/* printk() */
#include <linux/export.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */

#include <linux/mm.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <linux/kdev_t.h>
#include <asm/page.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/rwsem.h>

/*******************************************************************************
  Local head files
*/

#include "shm_type.h"
#include "shm_device.h"
#include "shm_api.h"
#include "memory_engine.h"
/*******************************************************************************
  Module Variable
*/


void MV_SHM_Check_Clean_Map(pid_t pid)
{

}


static void MV_SHM_insert_phyaddress_node(struct rb_root *shm_root,
						shm_address_t *shm_node)
{
	struct rb_node **p = &shm_root->rb_node;
	struct rb_node *parent = NULL;
	shm_address_t *tmp_node;

	while (*p) {
		parent = *p;
		tmp_node = rb_entry(parent, shm_address_t, phys_node);

		if (shm_node->m_phyaddress < tmp_node->m_phyaddress)
			p = &parent->rb_left;
		else
			p = &parent->rb_right;
	}

	rb_link_node(&shm_node->phys_node, parent, p);
	rb_insert_color(&shm_node->phys_node, shm_root);
}


static shm_address_t *MV_SHM_lookup_phyaddress_node(
	struct rb_root *shm_root, const uint address)
{
	struct rb_node *n = shm_root->rb_node;
	shm_address_t *tmp_node;

	while (n) {
		tmp_node = rb_entry(n, shm_address_t, phys_node);
		if (address < tmp_node->m_phyaddress) {
			n = n->rb_left;
		} else if (address > tmp_node->m_phyaddress) {
			if (address < (tmp_node->m_phyaddress
					+ tmp_node->m_size) ) {
				return tmp_node;
			} else {
				n = n->rb_right;
			}
		} else {
			return tmp_node;
		}
	}
	return NULL;
}


static void MV_SHM_delete_phyaddress_node(struct rb_root *shm_root,
						shm_address_t *shm_node)
{
	rb_erase(&shm_node->phys_node, shm_root);
}


static void MV_SHM_insert_virtaddress_node(struct rb_root *shm_root,
						shm_address_t *shm_node)
{
	struct rb_node **p = &shm_root->rb_node;
	struct rb_node *parent = NULL;
	shm_address_t *tmp_node;

	while (*p) {
		parent = *p;
		tmp_node = rb_entry(parent, shm_address_t, virt_node);

		if (shm_node->m_virtaddress < tmp_node->m_virtaddress)
			p = &parent->rb_left;
		else
			p = &parent->rb_right;
	}

	rb_link_node(&shm_node->virt_node, parent, p);
	rb_insert_color(&shm_node->virt_node, shm_root);
}


static shm_address_t *MV_SHM_lookup_virtaddress_node(
	struct rb_root *shm_root, const uint address)
{
	struct rb_node *n = shm_root->rb_node;
	shm_address_t *tmp_node;

	while (n) {
		tmp_node = rb_entry(n, shm_address_t, virt_node);
		if (address < tmp_node->m_virtaddress) {
			n = n->rb_left;
		} else if (address > tmp_node->m_virtaddress) {
			if (address < (tmp_node->m_virtaddress
					+ tmp_node->m_size) ) {
				return tmp_node;
			} else {
				n = n->rb_right;
			}
		} else {
			return tmp_node;
		}
	}

	return NULL;
}


static void MV_SHM_delete_virtaddress_node(struct rb_root *shm_root,
						shm_address_t *shm_node)
{
	rb_erase(&shm_node->virt_node, shm_root);
}


static void* MV_SHM_ioremap(unsigned long phys_start, unsigned long size)
{
	struct vm_struct *area;

	area = get_vm_area(size, VM_IOREMAP);
	if (area == NULL)
		return NULL;

	area->phys_addr = phys_start;
	if (ioremap_page_range((unsigned long)area->addr,
		(unsigned long)area->addr + size, phys_start, PAGE_KERNEL)) {
		return NULL;
	}
	return area->addr;
}


static void* MV_SHM_noncache_ioremap(unsigned long phys_start,
						unsigned long size)
{
	return (void*)ioremap_nocache(phys_start, size);
}

static size_t MV_SHM_Base_ioremap(unsigned long phys_start,
				unsigned long size, int mem_type)
{
	size_t virtaddress;
	if (mem_type == SHM_CACHE) {
		virtaddress = (size_t)MV_SHM_ioremap(phys_start, size);
	} else if (mem_type == SHM_NONCACHE) {
		virtaddress = (size_t)MV_SHM_noncache_ioremap(phys_start, size);
	} else {
		virtaddress = (size_t)NULL;
	}
	return virtaddress;
}

int MV_SHM_iounmap_and_free_rbtree(shm_device_t *device, size_t Offset)
{
	shm_address_t *address_node;

	if (device == NULL) {
		shm_error("kernel MV_SHM_iounmap_and_free_rbtree error\n");
		return -ENODEV;
	}

	down_read(&device->m_rwsem);
	address_node = MV_SHM_lookup_phyaddress_node(
		&device->m_phyaddr_root, (Offset + device->m_base));
	up_read(&device->m_rwsem);

	if (address_node ==NULL) {
		return 0;
	}

	down_write(&device->m_rwsem);
	MV_SHM_delete_phyaddress_node(&device->m_phyaddr_root, address_node);
	MV_SHM_delete_virtaddress_node(&device->m_virtaddr_root,address_node);
	up_write(&device->m_rwsem);

	iounmap((void *)(address_node->m_virtaddress));
	kfree(address_node);

	return 0;
}

static size_t MV_SHM_Base_Malloc(shm_device_t *shm_dev, size_t Size,
				size_t Alignment, int mem_type)
{
	size_t Offset;

	if ((shm_dev == NULL) || (Size == 0)
		|| (Alignment % 2)) {
		shm_error("kernel MV_SHM_Base_Malloc parameters error"
			"shm_dev[%p] size[%08x] align[%0x] mem_type[%d]\n",
			shm_dev, Size, Alignment, mem_type);
		return ERROR_SHM_MALLOC_FAILED;
	}

	if (shm_check_alignment(Alignment) != 0) {
		shm_error("kernel MV_SHM_Base_Malloc Alignment parameter"
			" error. align[%x]\n", Alignment);
		return ERROR_SHM_MALLOC_FAILED;
	}

	shm_round_size(Size);
	shm_round_alignment(Alignment);

	Offset = shm_device_allocate(shm_dev,
			Size, Alignment, mem_type);
	if (Offset == ERROR_SHM_MALLOC_FAILED) {

		shm_error("kernel MV_SHM_Base_Malloc malloc shm fail "
			"shm_dev[%p] size[%08x] align[%0x] mem_type[%d]\n",
			shm_dev, Size, Alignment, mem_type);
		return ERROR_SHM_MALLOC_FAILED;
	}

	return Offset;
}

static int MV_SHM_Base_Free(shm_device_t *shm_dev, size_t Offset)
{
	if (shm_dev == NULL)
		return -ENODEV;
	return shm_device_free(shm_dev, Offset);
}

static int MV_SHM_Base_Takeover(shm_device_t *shm_dev, size_t Offset)
{
	if (NULL == shm_dev)
		return -ENODEV;

	return shm_device_takeover(shm_dev, Offset);
}

static int MV_SHM_Base_Giveup(shm_device_t *shm_dev, size_t Offset)
{
	if (NULL == shm_dev)
		return -ENODEV;

	return shm_device_giveup(shm_dev, Offset);
}


static void *MV_SHM_Base_GetVirtAddr(shm_device_t *shm_dev,
					size_t Offset, int mem_type)
{
	shm_address_t *address_node;
	memory_node_t *tmp;
	size_t physaddress = 0;

	if (shm_dev == NULL) {
		shm_error("kernel MV_SHM_Base_GetVirtAddr parameters"
			" error shm_dev is NULL");
		return NULL;
	}

	if (Offset >= shm_dev->m_size) {
		shm_error("kernel MV_SHM_Base_GetVirtAddr parameters error"
			" shm_dev[%p] Offset[%08x] > shm_size[%08x] mem_type[%d]"
			"\n", shm_dev, Offset, shm_dev->m_size, mem_type);
		return NULL;
	}

	physaddress = Offset + shm_dev->m_base;

	mutex_lock(&(shm_dev->m_engine->m_mutex));
	tmp = memory_engine_lookup_shm_node_for_size(
		&(shm_dev->m_engine->m_shm_root), physaddress);
	mutex_unlock(&(shm_dev->m_engine->m_mutex));
	if (tmp == NULL) {
		shm_error("kernel MV_SHM_Base_GetVirtAddr"
			" memory_engine_lookup_shm_node_for_size"
			" offset[%08x] physaddress[%08x] mem_type[%d]\n",
			Offset, (physaddress), mem_type);
		return NULL;
	}

	down_write(&shm_dev->m_rwsem);
	address_node =
	MV_SHM_lookup_phyaddress_node(
		&(shm_dev->m_phyaddr_root), physaddress);

	if (address_node == NULL) {
		address_node = kmalloc(sizeof(shm_address_t), GFP_KERNEL);
		if(address_node == NULL) {
			up_write(&shm_dev->m_rwsem);
			shm_error("kernel MV_SHM_Base_GetVirtAddr"
				" kmalloc fail offset[%08x] mem_type[%d]\n",
				Offset, mem_type);
			return NULL;
		}
		address_node->m_phyaddress = tmp->m_phyaddress;
		address_node->m_size = MEMNODE_ALIGN_SIZE(tmp);
		address_node->m_virtaddress = (size_t)MV_SHM_Base_ioremap(
					address_node->m_phyaddress,
					address_node->m_size, mem_type);
		if (address_node->m_virtaddress == (size_t)NULL) {
			kfree(address_node);
			up_write(&shm_dev->m_rwsem);
			shm_error("kernel MV_SHM_Base_GetVirtAddr"
				" MV_SHM_ioremap fail offset[%08x]"
				" mem_type[%d]\n",Offset, mem_type);
			return NULL;
		}

		MV_SHM_insert_phyaddress_node(
			&(shm_dev->m_phyaddr_root), address_node);
		MV_SHM_insert_virtaddress_node(
			&(shm_dev->m_virtaddr_root), address_node);
		up_write(&shm_dev->m_rwsem);

		return (void *)(address_node->m_virtaddress +
				((Offset + shm_dev->m_base)
				- address_node->m_phyaddress));
	} else {
		if((address_node->m_phyaddress == tmp->m_phyaddress)
			&& (address_node->m_size == MEMNODE_ALIGN_SIZE(tmp))) {
			up_write(&shm_dev->m_rwsem);
			return (void *)(address_node->m_virtaddress +
				((Offset + shm_dev->m_base)
				- address_node->m_phyaddress));
		} else {
			MV_SHM_delete_phyaddress_node(
				&(shm_dev->m_phyaddr_root), address_node);
			MV_SHM_delete_virtaddress_node(
				&(shm_dev->m_virtaddr_root),address_node);

			iounmap((void *)(address_node->m_virtaddress));

			address_node->m_phyaddress = tmp->m_phyaddress;
			address_node->m_size = MEMNODE_ALIGN_SIZE(tmp);
			address_node->m_virtaddress = (size_t)MV_SHM_Base_ioremap(
						address_node->m_phyaddress,
						address_node->m_size, mem_type);
			if (address_node->m_virtaddress == (size_t)NULL) {
				kfree(address_node);
				up_write(&shm_dev->m_rwsem);
				shm_error("kernel MV_SHM_Base_GetVirtAddr"
					" MV_SHM_ioremap fail offset[%08x]"
					" mem_type[%d]\n", Offset, mem_type);
				return NULL;
			}

			MV_SHM_insert_phyaddress_node(
				&(shm_dev->m_phyaddr_root), address_node);
			MV_SHM_insert_virtaddress_node(
				&(shm_dev->m_virtaddr_root), address_node);
			up_write(&shm_dev->m_rwsem);
			return (void *)(address_node->m_virtaddress +
				((Offset + shm_dev->m_base)
				- address_node->m_phyaddress));
		}
	}
}


static void *MV_SHM_Base_GetPhysAddr(shm_device_t *shm_dev, size_t Offset)
{
	if (shm_dev == NULL) {
		shm_error("kernel MV_SHM_Base_GetPhysAddr parameter error"
			"shm dev is NULL");
		return NULL;
	}

	if (Offset >= shm_dev->m_size) {
		shm_error("kernel MV_SHM_Base_GetPhysAddr parameter error"
			"shm_dev[%p] offset[%08x] > shm_size[%08x]\n",
			shm_dev, Offset, shm_dev->m_size);
		return NULL;
	}

	return (void *)(Offset + shm_dev->m_base);
}

static size_t MV_SHM_Base_RevertVirtAddr(shm_device_t *shm_dev, void *ptr)
{
	shm_address_t *address_node;
	if (shm_dev == NULL) {
		shm_error("kernel MV_SHM_Base_RevertVirtAddr parameter error\n");
		return ERROR_SHM_MALLOC_FAILED;
	}

	down_read(&shm_dev->m_rwsem);
	address_node =
	MV_SHM_lookup_virtaddress_node(
		&(shm_dev->m_virtaddr_root), (size_t) ptr);
	up_read(&shm_dev->m_rwsem);

	if (address_node == NULL) {
		shm_error("kernel MV_SHM_RevertCacheVirtAddr"
			" fail [%08x]\n",(size_t) ptr);
		return ERROR_SHM_MALLOC_FAILED;
	} else {
		return (size_t)((address_node->m_phyaddress +
			(((size_t) ptr) - address_node->m_virtaddress))
			- shm_dev->m_base);
	}
}

static size_t MV_SHM_Base_RevertPhysAddr(shm_device_t *shm_dev, void *ptr)
{
	if (shm_dev == NULL) {
		shm_error("kernel MV_SHM_Base_RevertPhysAddr parameter error"
			" shm_dev[%p]\n", shm_dev);
		return ERROR_SHM_MALLOC_FAILED;
	}

	if (((size_t) ptr < shm_dev->m_base) ||
		((size_t) ptr >= shm_dev->m_base + shm_dev->m_size)) {
		shm_error("kernel MV_SHM_Base_RevertPhysAddr parameter error"
			" physaddr[%08x] shm_base[%08x] shm_size[%08x]\n",
			(size_t)ptr, shm_dev->m_base, shm_dev->m_size);
		return ERROR_SHM_MALLOC_FAILED;
	}

	return ((size_t) ptr - shm_dev->m_base);
}

static int MV_SHM_Base_GetMemory(shm_device_t *shm_dev,
					size_t offset, int mem_type)
{
	if (shm_dev == NULL) {
		shm_error("kernel MV_SHM_Base_GetMemory parameter error"
			" shm_dev[%p] offset[%08x] mem_type[%d]\n",
			shm_dev, offset, mem_type);
		return -1;
	}

	if (shm_device_add_reference_count(shm_dev,offset) != 0) {
		shm_error("kernel MV_SHM_Base_GetMemory "
			"shm_device_add_reference_count fail"
			" offset[%08x] mem_type[%d]\n", offset, mem_type);
		return -1;
	}

	if (MV_SHM_Base_GetVirtAddr(shm_dev, offset, mem_type) == NULL) {
		shm_error("kernel MV_SHM_Base_GetMemory fail\n");
		return -1;
	}

	return 0;
}

static int MV_SHM_Base_PutMemory(shm_device_t *shm_dev, size_t offset)
{
	MV_SHM_iounmap_and_free_rbtree(shm_dev, offset);
	return MV_SHM_Base_Free(shm_dev, offset);
}

/*******************************************************************************
  Module API: for kernel module use only, mainly CBuf and Graphics driver, suppose to be removed later
*/
int MV_SHM_Init(shm_device_t * device_secure, shm_device_t * device_cache)
{
	if (device_cache == NULL)
		return -EINVAL;

	init_rwsem(&device_cache->m_rwsem);
	device_cache->m_phyaddr_root = RB_ROOT;
	device_cache->m_virtaddr_root = RB_ROOT;

	init_rwsem(&device_secure->m_rwsem);
	device_secure->m_phyaddr_root = RB_ROOT;
	device_secure->m_virtaddr_root = RB_ROOT;
	return 0;
}

int MV_SHM_Exit(void)
{
	return 0;
}


int MV_SHM_GetCacheMemInfo(pMV_SHM_MemInfo_t pInfo)
{
	if (pInfo == NULL)
		return -EINVAL;

	if (shm_device == NULL)
		return -ENODEV;

	return shm_device_get_meminfo(shm_device, pInfo);
}


int MV_SHM_GetCacheBaseInfo(pMV_SHM_BaseInfo_t pInfo)
{
	if (pInfo == NULL)
		return -EINVAL;

	if (shm_device == NULL)
		return -ENODEV;

	pInfo->m_base_physaddr = shm_device->m_base;
	pInfo->m_base_virtaddr = 0;
	pInfo->m_fd = -1;
	pInfo->m_size = shm_device->m_size;
	pInfo->m_threshold = shm_device->m_threshold;

	return 0;
}


size_t MV_SHM_Malloc(size_t Size, size_t Alignment)
{
	return MV_SHM_Base_Malloc(shm_device, Size, Alignment,SHM_CACHE);
}


int MV_SHM_Takeover(size_t Offset)
{
	return MV_SHM_Base_Takeover(shm_device, Offset);
}


int MV_SHM_Giveup(size_t Offset)
{
	return MV_SHM_Base_Giveup(shm_device, Offset);
}


int MV_SHM_Free(size_t Offset)
{
	MV_SHM_iounmap_and_free_rbtree(shm_device, Offset);
	return MV_SHM_Base_Free(shm_device, Offset);
}


size_t MV_SHM_NONCACHE_Malloc(size_t Size, size_t Alignment)
{
	return MV_SHM_Base_Malloc(shm_device, Size, Alignment,SHM_NONCACHE);
}


int MV_SHM_NONCACHE_Free(size_t Offset)
{
	MV_SHM_iounmap_and_free_rbtree(shm_device, Offset);
	return MV_SHM_Base_Free(shm_device, Offset);
}


void *MV_SHM_GetNonCacheVirtAddr(size_t Offset)
{
	return MV_SHM_Base_GetVirtAddr(shm_device, Offset, SHM_NONCACHE);
}


void *MV_SHM_GetCacheVirtAddr(size_t Offset)
{
	return MV_SHM_Base_GetVirtAddr(shm_device, Offset, SHM_CACHE);
}


void *MV_SHM_GetNonCachePhysAddr(size_t Offset)
{
	return MV_SHM_Base_GetPhysAddr(shm_device, Offset);
}


void *MV_SHM_GetCachePhysAddr(size_t Offset)
{
	return MV_SHM_Base_GetPhysAddr(shm_device, Offset);
}


size_t MV_SHM_RevertNonCacheVirtAddr(void *ptr)
{
	return MV_SHM_Base_RevertVirtAddr(shm_device, ptr);
}


size_t MV_SHM_RevertCacheVirtAddr(void *ptr)
{
	return MV_SHM_Base_RevertVirtAddr(shm_device, ptr);
}


size_t MV_SHM_RevertNonCachePhysAddr(void *ptr)
{
	return MV_SHM_Base_RevertPhysAddr(shm_device, ptr);
}


size_t MV_SHM_RevertCachePhysAddr(void *ptr)
{
	return MV_SHM_Base_RevertPhysAddr(shm_device, ptr);
}


int MV_SHM_GetMemory(size_t offset)
{
	return MV_SHM_Base_GetMemory(shm_device, offset, SHM_CACHE);
}


int MV_SHM_PutMemory(size_t offset)
{
	return MV_SHM_Base_PutMemory(shm_device, offset);
}


int MV_SHM_NONCACHE_GetMemory(size_t offset)
{
	return MV_SHM_Base_GetMemory(shm_device, offset, SHM_NONCACHE);
}


int MV_SHM_NONCACHE_PutMemory(size_t offset)
{
	return MV_SHM_Base_PutMemory(shm_device, offset);
}


EXPORT_SYMBOL(MV_SHM_Malloc);
EXPORT_SYMBOL(MV_SHM_Free);
EXPORT_SYMBOL(MV_SHM_Takeover);
EXPORT_SYMBOL(MV_SHM_Giveup);
EXPORT_SYMBOL(MV_SHM_NONCACHE_Malloc);
EXPORT_SYMBOL(MV_SHM_NONCACHE_Free);
EXPORT_SYMBOL(MV_SHM_GetNonCacheVirtAddr);
EXPORT_SYMBOL(MV_SHM_GetCacheVirtAddr);
EXPORT_SYMBOL(MV_SHM_GetNonCachePhysAddr);
EXPORT_SYMBOL(MV_SHM_GetCachePhysAddr);
EXPORT_SYMBOL(MV_SHM_RevertNonCacheVirtAddr);
EXPORT_SYMBOL(MV_SHM_RevertCacheVirtAddr);
EXPORT_SYMBOL(MV_SHM_RevertNonCachePhysAddr);
EXPORT_SYMBOL(MV_SHM_RevertCachePhysAddr);
EXPORT_SYMBOL(MV_SHM_GetCacheMemInfo);
EXPORT_SYMBOL(MV_SHM_GetCacheBaseInfo);
EXPORT_SYMBOL(MV_SHM_Check_Clean_Map);
EXPORT_SYMBOL(MV_SHM_GetMemory);
EXPORT_SYMBOL(MV_SHM_PutMemory);
EXPORT_SYMBOL(MV_SHM_NONCACHE_GetMemory);
EXPORT_SYMBOL(MV_SHM_NONCACHE_PutMemory);
