/**************************************************************************
*
* Copyright 2011-2015 by Andrey Butok. FNET Community.
* Copyright 2008-2010 by Andrey Butok. Freescale Semiconductor, Inc.
*
***************************************************************************
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License Version 3 
* or later (the "LGPL").
*
* As a special exception, the copyright holders of the FNET project give you
* permission to link the FNET sources with independent modules to produce an
* executable, regardless of the license terms of these independent modules,
* and to copy and distribute the resulting executable under terms of your 
* choice, provided that you also meet, for each linked independent module,
* the terms and conditions of the license of that module.
* An independent module is a module which is not derived from or based 
* on this library. 
* If you modify the FNET sources, you may extend this exception 
* to your version of the FNET sources, but you are not obligated 
* to do so. If you do not wish to do so, delete this
* exception statement from your version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* You should have received a copy of the GNU General Public License
* and the GNU Lesser General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*
**********************************************************************//*!
*
* @file fnet_mempool.c
*
* @brief FNET memory pool functions.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_mempool.h"
#include "fnet_isr.h"

#define FNET_MEMPOOL_MALLOC_BEST_CHOICE (1) /* Choose the best. */

#define FNET_DEBUG_MEMPOOL_CHECK        (0)

struct fnet_mempool
{
    fnet_mempool_unit_header_t *    free_ptr;
    fnet_size_t                     unit_size;
};

#if FNET_DEBUG_MEMPOOL_CHECK
    #define DEBUG_ALLOCATED_ADDR_MAX    200

    struct DEBUG_allocated_addr_entry
    {
        void *address;
        fnet_size_t size;
    };

    struct DEBUG_allocated_addr_entry DEBUG_allocated_addr[DEBUG_ALLOCATED_ADDR_MAX];
	fnet_int32_t DEBUG_last_free_addr_num = 0;
#endif 

/************************************************************************
* NAME: fnet_mempool_init
*
* DESCRIPTION:  
*************************************************************************/
fnet_mempool_desc_t fnet_mempool_init( void *pool_ptr, fnet_size_t pool_size, fnet_mempool_align_t alignment )
{
    struct fnet_mempool * mempool = 0;
    
    if(alignment < FNET_MEMPOOL_ALIGN_8)
    {
        alignment = FNET_MEMPOOL_ALIGN_8; /* Set default alignment. */ 
    }
    
    if(pool_ptr && (pool_size > (fnet_size_t)(alignment+sizeof(struct fnet_mempool))))
    {
        fnet_mempool_unit_header_t  *p;
        fnet_uint8_t               *heap_ptr = (fnet_uint8_t *)(((fnet_uint32_t)(pool_ptr)+sizeof(struct fnet_mempool)+((fnet_uint32_t)alignment+1u))
                                                            & (0xffffffffu-alignment));
        fnet_size_t                 heap_size = pool_size - ((fnet_uint32_t)heap_ptr - (fnet_uint32_t)pool_ptr);
        
        mempool = (struct fnet_mempool *) pool_ptr;
        
        mempool->unit_size = (fnet_size_t)alignment+1u;
        
        
        p = (fnet_mempool_unit_header_t *)heap_ptr;
        p->size = (heap_size / mempool->unit_size);
        p->ptr = p;
        mempool->free_ptr = p;
    }
    
    return (fnet_mempool_desc_t)mempool;
}

/************************************************************************
* NAME: fnet_mem_release
*
* DESCRIPTION: Release the mempool.
*              
*************************************************************************/
void fnet_mempool_release( fnet_mempool_desc_t mpool )
{
    struct fnet_mempool * mempool = (struct fnet_mempool *)mpool;
    mempool->free_ptr = 0;
}

/************************************************************************
* NAME: fnet_free
*
* DESCRIPTION: Frees memory in the mempool.
*              
*************************************************************************/
void fnet_mempool_free( fnet_mempool_desc_t mpool, void *ap )
{
    struct fnet_mempool * mempool = (struct fnet_mempool *)mpool;
    
    fnet_mempool_unit_header_t *bp, *p;

    if(ap != 0)
    {
        fnet_isr_lock();
      
        /* Block pointer = allocated memory block addr - allocation unit size.*/
        bp = (fnet_mempool_unit_header_t *)((fnet_uint32_t)ap - mempool->unit_size); /* Point to block header. */ 


#if FNET_DEBUG_MEMPOOL_CHECK
    {
        fnet_index_t i;
        
        for(i=0u; i<DEBUG_ALLOCATED_ADDR_MAX; i++)
        {
            if(DEBUG_allocated_addr[i].address == ap)
            {
                if(DEBUG_allocated_addr[i].size == bp->size)
                {
                    DEBUG_allocated_addr[i].address = 0; // Clear;
                    DEBUG_last_free_addr_num = i-50;
                    if (DEBUG_last_free_addr_num < 0)
                         DEBUG_last_free_addr_num = 0;
                    break;
                }
                else
                {
                    fnet_println ("Wrong free size!!!!!!!!");
                    break;
                }
            }
        
        }
        if( i==DEBUG_ALLOCATED_ADDR_MAX)
            fnet_println ("This addr never allocated before!!!!!!!!");
    }
#endif

        for (p = mempool->free_ptr; !((bp > p) && (bp < p->ptr)); p = p->ptr)
        {
            
#if FNET_DEBUG_MEMPOOL_CHECK /* Debug Check. */
            if( (p <= bp) && ((fnet_uint32_t)bp <= ((fnet_uint32_t)p + p->size)) )/* The block is already free */ 
            {
                fnet_println("Already Free UPS");
                fnet_isr_unlock();
                return; 
            }
#endif            
                
            
            if((p >= p->ptr) && ((bp > p) || (bp < p->ptr)))
            {
                break; /* Freed block at start or end of arena. */
            }
        }

        if((fnet_mempool_unit_header_t *)((fnet_uint32_t)bp + bp->size*mempool->unit_size) == p->ptr)
        {
            bp->size += p->ptr->size;
            bp->ptr = p->ptr->ptr;
        }
        else
        {
            bp->ptr = p->ptr;
        }

        if((fnet_mempool_unit_header_t *)((fnet_uint32_t)p + p->size*mempool->unit_size) == bp)
        {
            p->size += bp->size;
            p->ptr = bp->ptr;
        }
        else
        {
            p->ptr = bp;
        }
        
        mempool->free_ptr = p;
 
        fnet_isr_unlock();
    }

}

/************************************************************************
* NAME: fnet_mempool_malloc
*
* DESCRIPTION: Allocates memory in the memory pool.
*              
*************************************************************************/
#if FNET_MEMPOOL_MALLOC_BEST_CHOICE /* Choose the best. */
void *fnet_mempool_malloc(fnet_mempool_desc_t mpool, fnet_size_t nbytes )
{
    struct fnet_mempool         *mempool = (struct fnet_mempool *)mpool;
    fnet_mempool_unit_header_t  *p, *prevp;
    fnet_mempool_unit_header_t  *best_p = 0;
    fnet_mempool_unit_header_t  *best_p_prev = 0;
    fnet_size_t                 nunits;
    void                        *res;

    fnet_isr_lock();

    nunits = ((nbytes + mempool->unit_size - 1u) / mempool->unit_size) + 1u;

    prevp = mempool->free_ptr;
    
    best_p_prev = prevp;
    
    /* Find the best one. */
    p = prevp->ptr;
    for(;;)
    {
        if( (p->size >= nunits) && ( (best_p==0) || ((best_p)&&(best_p->size > p->size)) ) )
        {
            best_p_prev = prevp;
            best_p = p;  
        }
        
        if(p == mempool->free_ptr)
        {
            break; /* End of list is reached. */
        }
    
        prevp = p;
        p = p->ptr;
    }

    if(best_p)
    {
        if(best_p->size == nunits)
        {
            best_p_prev->ptr = best_p->ptr;
        }
        else
        {
            best_p->size -= nunits; /* Put to the top. */
            best_p = (fnet_mempool_unit_header_t *)((fnet_uint32_t)best_p + best_p->size*mempool->unit_size);
            best_p->size = nunits;
        }

        mempool->free_ptr = best_p_prev;
        res = (void *)((fnet_uint32_t)best_p + mempool->unit_size);
#if 0 /* Clear mem.*/
        fnet_memset_zero( res, (nunits-1)* mempool->unit_size ); 
#endif  
    }
    else
    {
		/* Break here to detect allocation errors */
        res = 0;
    }

#if FNET_DEBUG_MEMPOOL_CHECK    
    if(res)
    {
        fnet_index_t i;
        
        for(i=DEBUG_last_free_addr_num; i<DEBUG_ALLOCATED_ADDR_MAX; i++)
        {
            if(DEBUG_allocated_addr[i].address == 0)
            {
                /* Save allocated address */
                DEBUG_allocated_addr[i].address = res;
                DEBUG_allocated_addr[i].size = nunits;
                
                break;
            }
        
        }
        if( i==DEBUG_ALLOCATED_ADDR_MAX)
            fnet_println ("Addr_table_overload!!!");
    }
#endif
    
    fnet_isr_unlock();
    
    return res;
}

#else /* Choose the first. */

void *fnet_mempool_malloc(fnet_mempool_desc_t mpool, fnet_size_t nbytes )
{
    struct fnet_mempool         *mempool = (struct fnet_mempool *)mpool;
    fnet_mempool_unit_header_t  *p, *prevp;
    fnet_size_t                 nunits;

    fnet_isr_lock();

    nunits = ((nbytes + mempool->unit_size - 1) / mempool->unit_size) + 1;

    prevp = mempool->free_ptr;

    for (p = prevp->ptr; ; prevp = p, p = p->ptr)
    {
        if(p->size >= nunits)
        {
            if(p->size == nunits)
            {
                prevp->ptr = p->ptr;
            }
            else
            {
                p->size -= nunits; /* Put to the top. */
                p = (fnet_mempool_unit_header_t *)((fnet_uint32_t)p + p->size*mempool->unit_size);
                p->size = nunits;
            }

            mempool->free_ptr = prevp;

            fnet_isr_unlock();
            return (void *)((fnet_uint32_t)p + mempool->unit_size);
        }

        if(p == mempool->free_ptr)
        {
            fnet_isr_unlock();
            return 0;
        }
    }

    
    fnet_isr_unlock();
    return 0;
}

#endif

/************************************************************************
* NAME: fnet_free_mem_status
*
* DESCRIPTION: Returns a quantity of free memory (for debug needs)
*              
*************************************************************************/
fnet_size_t fnet_mempool_free_mem_status( fnet_mempool_desc_t mpool)
{
    struct fnet_mempool         * mempool = (struct fnet_mempool *)mpool;
    fnet_size_t                 total_size = 0u;
    fnet_mempool_unit_header_t  *t_mem;

    fnet_isr_lock();

    t_mem = mempool->free_ptr;

    if(t_mem)
    {
        total_size += t_mem->size;
        #if 0
         FNET_DEBUG("%d,",t_mem->size*sizeof(FNET_ALLOC_HDR_T));
        #endif

        while(t_mem->ptr != mempool->free_ptr)
        {
            t_mem = t_mem->ptr;
            total_size += t_mem->size;
        #if 0
            FNET_DEBUG("%d,",t_mem->size*sizeof(FNET_ALLOC_HDR_T));*/
        #endif
        }
    }

    fnet_isr_unlock();

    return (total_size * mempool->unit_size);
}

/************************************************************************
* NAME: fnet_malloc_max
*
* DESCRIPTION: Returns a maximum size of posible allocated memory chunk.
*              
*************************************************************************/
fnet_size_t fnet_mempool_malloc_max( fnet_mempool_desc_t mpool )
{
    struct fnet_mempool         *mempool = (struct fnet_mempool *)mpool;
    fnet_size_t                 max = 0u;
    fnet_mempool_unit_header_t  *t_mem;

    fnet_isr_lock();

    t_mem = mempool->free_ptr;

    if(t_mem)
    {

        max = t_mem->size;
    #if 0
        FNET_DEBUG("%d,", t_mem->size * sizeof(FNET_ALLOC_HDR_T));
    #endif

        while((t_mem->ptr) && (t_mem->ptr != mempool->free_ptr))
        {
            t_mem = t_mem->ptr;

            if(t_mem->size > max)
            {
                max = t_mem->size;
            }    
        #if 0
            FNET_DEBUG("%d,", t_mem->size * sizeof(FNET_ALLOC_HDR_T));
        #endif
        }
    #if 0
        FNET_DEBUG("");
    #endif
    }

    fnet_isr_unlock();

    return (max * mempool->unit_size);
}

#if 0 /* For Debug needs.*/
fnet_return_t fnet_mempool_check( fnet_mempool_desc_t mpool )
{

    volatile struct fnet_mempool * mempool = (struct fnet_mempool *)mpool;
    volatile fnet_mempool_unit_header_t *t_mem;
    fnet_index_t i = 0u;

    fnet_isr_lock();

    t_mem = mempool->free_ptr;

    if(t_mem)
    {

        while(t_mem->ptr != mempool->free_ptr)
        {
            i++;
            
            if((i>100u)||(t_mem->ptr == 0)||(t_mem->ptr == (void *)0xFFFFFFFF))
            {
                fnet_println("!!!MEMPOOL CRASH!!!");
                return FNET_ERR;
            }
            
            if( (((fnet_uint32_t)t_mem) < (fnet_uint32_t)t_mem->ptr) && ((t_mem->size + (fnet_uint32_t)t_mem) > (fnet_uint32_t)t_mem->ptr))
            {
                fnet_println("!!!MEMPOOL FREE CRASH!!!");
                return FNET_ERR;
            }
            
            t_mem = t_mem->ptr;

        }
    }


    fnet_isr_unlock();
    return FNET_OK;
}
#endif
 
