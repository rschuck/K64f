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
**********************************************************************/ /*!
*
* @file fnet_netbuf.h
*
* @author Andrey Butok
*
* @brief Private. FNET Buffer and memory management definitions.
*
***************************************************************************/

#ifndef _FNET_NETBUF_H_

#define _FNET_NETBUF_H_

#include "fnet_mempool.h"

/**************************************************************************/ /*!
 * @internal
 * @brief    netbuf flags.
 ******************************************************************************/
typedef enum
{
    FNET_NETBUF_FLAG_NONE                   = 0x00,     /* None. */
    FNET_NETBUF_FLAG_BROADCAST              = 0x02,     /* Send/received as link-level broadcast. */
    FNET_NETBUF_FLAG_MULTICAST              = 0x04,     /* Send/received as link-level multicast. */
    FNET_NETBUF_FLAG_HW_IP_CHECKSUM         = 0x10,     /* IPv4 header checksum is calculated/checked by HW.*/
    FNET_NETBUF_FLAG_HW_PROTOCOL_CHECKSUM   = 0x20      /* Protocol (UDP, TCP, ICMP) checksum is calculated/checked by HW.*/
} fnet_netbuf_flag_t;

/**************************************************************************/ /*!
 * @internal
 * @brief    netbuf types.
 ******************************************************************************/
typedef enum
{
    FNET_NETBUF_TYPE_DATA    = 0,       /**< dynamic (data) allocation.*/
    FNET_NETBUF_TYPE_HEADER  = 1,
    FNET_NETBUF_TYPE_ADDRESS = 2,       /**< address of the message.*/
    FNET_NETBUF_TYPE_OPTION  = 3        /**< options.*/
} fnet_netbuf_type_t;

/**************************************************************************/ /*!
 * @internal
 * @brief    Header at beginning of each net_buf.
 ******************************************************************************/
typedef struct fnet_netbuf
{
    struct fnet_netbuf  *next;          /**< next buffer in chain */
    struct fnet_netbuf  *next_chain;    /**< next chain in queue/record */
    void                *data;          /**< pointer to the beginning of the data buffer */
    void                *data_ptr;      /**< pointer to actual data */
    fnet_size_t         length;         /**< amount of actual data in this net_buf */
    fnet_size_t         total_length;   /**< length of buffer + additionally chained buffers (only for first netbuf)*/
    fnet_flag_t         flags;
} fnet_netbuf_t;

#define FNET_NETBUF_COPYALL   ((fnet_size_t)(-1))

#if defined(__cplusplus)
extern "C" {
#endif

/* Memory management functions */
fnet_return_t fnet_heap_init( void *heap_ptr, fnet_size_t heap_size );
void fnet_free( void *ap );
void *fnet_malloc( fnet_size_t nbytes );
void *fnet_malloc_zero( fnet_size_t nbytes );
fnet_size_t fnet_free_mem_status( void );
fnet_size_t fnet_malloc_max( void );
void fnet_mem_release( void );

void fnet_free_netbuf( void *ap );
void *fnet_malloc_netbuf( fnet_size_t nbytes );
fnet_size_t fnet_free_mem_status_netbuf( void );
fnet_size_t fnet_malloc_max_netbuf( void );
void fnet_mem_release_netbuf( void );

/* Netbuf service routines */
fnet_netbuf_t *fnet_netbuf_new( fnet_size_t len, fnet_bool_t drain );
fnet_netbuf_t *fnet_netbuf_free( fnet_netbuf_t *nb );
fnet_netbuf_t *fnet_netbuf_copy( fnet_netbuf_t *nb, fnet_size_t offset, fnet_size_t len, fnet_bool_t drain );
fnet_netbuf_t *fnet_netbuf_from_buf( void *data_ptr, fnet_size_t len, fnet_bool_t drain );
fnet_netbuf_t *fnet_netbuf_concat( fnet_netbuf_t *nb1, fnet_netbuf_t *nb2 );
void fnet_netbuf_to_buf( fnet_netbuf_t *nb, fnet_size_t offset, fnet_size_t len, void *data_ptr );
fnet_return_t fnet_netbuf_pullup( fnet_netbuf_t **nb_ptr, fnet_size_t len);
void fnet_netbuf_trim( fnet_netbuf_t ** nb_ptr, fnet_int32_t len );
fnet_netbuf_t *fnet_netbuf_cut_center( fnet_netbuf_t ** nb_ptr, fnet_size_t offset, fnet_size_t len);
void fnet_netbuf_add_chain( fnet_netbuf_t ** nb_ptr, fnet_netbuf_t *nb_chain );
void fnet_netbuf_del_chain( fnet_netbuf_t ** nb_ptr, fnet_netbuf_t *nb_chain );
void fnet_netbuf_free_chain( fnet_netbuf_t *nb );
                                             
#if 0 /* For Debug needs.*/                                            
fnet_return_t fnet_netbuf_mempool_check( void ); 
void FNET_DEBUG_NETBUF_print_chain( fnet_netbuf_t *nb, fnet_uint8_t *str, fnrt_index_t max);                                       
#endif

#if 0 /* For Debug needs.*/
    void fnet_free_netbuf_low( void *ap );
    #define fnet_free_netbuf(ap )   do{ fnet_free_netbuf_low( ap ); \
                                                ap = (void *)0x0; \
                                            } while(0)
#endif

#if defined(__cplusplus)
}
#endif

#endif
