/*   
	Custom IOS module for Wii.
        OH0 message loop
    Copyright (C) 2009 kwiirk.
    Copyright (C) 2008 neimod.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


/*******************************************************************************
 *
 * oh0_loop.c - IOS module main code
 * even if we are "ehc" driver, we still pretend to be "oh0"
 * and implement "standard" ios oh0 usb api
 *
 *******************************************************************************
 *
 */



#include <stdio.h>
#include <string.h>
#include "syscalls.h"
#include "ehci_types.h"
#include "ehci.h"
#include "utils.h"
#include "libwbfs.h"
#include "ehci_interrupt.h"

void ehci_usleep(int usec);
void ehci_msleep(int msec);

#undef NULL
#define NULL ((void *)0)
#define IOS_OPEN				0x01
#define IOS_CLOSE				0x02
#define IOS_IOCTL				0x06
#define IOS_IOCTLV				0x07

#define USB_IOCTL_CTRLMSG			0
#define USB_IOCTL_BLKMSG			1
#define USB_IOCTL_INTRMSG			2
#define USB_IOCTL_SUSPENDDEV			5
#define USB_IOCTL_RESUMEDEV			6
#define USB_IOCTL_GETDEVLIST			12
#define USB_IOCTL_DEVREMOVALHOOK		26
#define USB_IOCTL_DEVINSERTHOOK			27

#define UMS_BASE (('U'<<24)|('M'<<16)|('S'<<8))
#define USB_IOCTL_UMS_INIT	        	(UMS_BASE+0x1)
#define USB_IOCTL_UMS_GET_CAPACITY      	(UMS_BASE+0x2)
#define USB_IOCTL_UMS_READ_SECTORS      	(UMS_BASE+0x3)
#define USB_IOCTL_UMS_WRITE_SECTORS		(UMS_BASE+0x4)

#define USB_IOCTL_UMS_READ_STRESS		(UMS_BASE+0x5)

#define USB_IOCTL_UMS_SET_VERBOSE		(UMS_BASE+0x6)

#define USB_IOCTL_UMS_UMOUNT			(UMS_BASE+0x10)
#define USB_IOCTL_UMS_WATCHDOG			(UMS_BASE+0x80)

#define USB_IOCTL_UMS_TESTMODE			(UMS_BASE+0x81)


#define WBFS_BASE (('W'<<24)|('F'<<16)|('S'<<8))
#define USB_IOCTL_WBFS_OPEN_DISC	        (WBFS_BASE+0x1)
#define USB_IOCTL_WBFS_READ_DISC	        (WBFS_BASE+0x2)
#define USB_IOCTL_WBFS_READ_DIRECT_DISC	    (WBFS_BASE+0x3)
#define USB_IOCTL_WBFS_STS_DISC				(WBFS_BASE+0x4)

//#define USB_IOCTL_WBFS_SPEED_LIMIT			(WBFS_BASE+0x80)

void USBStorage_Umount(void);

//#define DEVICE "/dev/usb/ehc"
#define DEVICE "/dev/usb2"

int verbose = 0;
#define ioctlv_u8(a) (*((u8*)(a).data))
#define ioctlv_u16(a) (*((u16*)(a).data))
#define ioctlv_u32(a) (*((u32*)(a).data))
#define ioctlv_len(a) (a).len
#define ioctlv_voidp(a) (a).data

wbfs_disc_t * wbfs_init_with_partition(u8*discid, int partition);


int USBStorage_DVD_Test(void);

#define WATCHDOG_TIMER 1000*1000*10


int test_mode=0;

char *parse_hex(char *base,int *val)
{
        int v = 0,done=0;
        char *ptr = base,c;
        while(!done)
        {
                c = *ptr++;
                if(c >= '0' &&  c <= '9')
                        v = v << 4 | (c-'0');
                else if(c >= 'a' &&  c <= 'f')
                        v = v << 4 | (10+c-'a');
                else if(c >= 'A' &&  c <= 'F')
                        v = v << 4 | (10+c-'A');
                else
                        done = 1;
        }
        if(ptr==base+1)//only incremented once
                return 0; //we did not found any hex numbers
        *val = v;
        return ptr-1;
}
int parse_and_open_device(char *devname,int fd)
{
        char *ptr = devname;
        int vid,pid;
        if (! (ptr = parse_hex(ptr,&vid)))
                return -6;
        if ( *ptr != '/' )
                return -6;
        ptr++;// skip /
        if (! (ptr = parse_hex(ptr,&pid)))
                return -6;
        if ( *ptr != '\0' )
                return -6;
        return ehci_open_device(vid,pid,fd);
}


int DVD_speed_limit=0; // ingame it can fix x6 speed

int watchdog_enable=1;

// special ingame
int wbfs_disc_read2(wbfs_disc_t*d,u32 offset, u8 *data, u32 len);

// heap space for WBFS  and queue

extern int heaphandle;

void msleep(int msec);

u8 mem_sector[4096] __attribute__ ((aligned (32)));

void *WBFS_Alloc(int size)
{
  void * ret = 0;
  ret= os_heap_alloc(heaphandle, size);
  if(ret==0)
	{debug_printf("WBFS not enough memory! need %d\n",size);
    os_puts("WBFS not enough memory!\n");
    while(1) ehci_msleep(100);
	}
  return ret;
}
void WBFS_Free(void *ptr)
{
        return os_heap_free(heaphandle, ptr);
}

extern u8 *disc_buff;

u32 last_disc_lba=0;
u32 current_disc_lba=0xffffffff;


void wbfs_perform_disc(void);

// offset -> disc_offset in words 
// data -> buffer
// len -> len to read in bytes

int WBFS_direct_disc_read(u32 offset, u8 *data, u32 len)
{
int r=true;
u32 lba;
u32 len2=len;
u8* data2=data;
u32 sec_size;
int l;
u8 *buff;

	os_sync_after_write(data2, len2);

	if(!disc_buff) return 0x8000;

	 
	last_disc_lba= USBStorage_Get_Capacity(&sec_size);

	if(last_disc_lba==0 || sec_size!=2048)
		{
		current_disc_lba=0xffffffff;
		return 0x8000;
		}

	buff=(u8 *) (((u32)disc_buff+31) & ~31); // 32 bytes aligment
	   
	while(len>0)
		{
		lba=offset>>9; // offset to LBA (sector have 512 words)

		if((lba & ~15)!=current_disc_lba)
			{
			current_disc_lba=(lba & ~15);
			l=(last_disc_lba-current_disc_lba);if(l>16) l=16;

			if(l<16) memset(buff,0,0x8000);
			if(l>0)
				{
				r=USBStorage_Read_Sectors(current_disc_lba, l, buff); // read 16 cached sectors
				if(!r) break;
				}
			}
		
		l=0x8000-((offset & 8191)<<2); // get max size in the cache relative to offset
		if(l>len) l=len;

		memcpy(data, &buff[((offset & 8191)<<2)], l);
		os_sync_after_write(data, l);

		data+=l;
		len-=l;
		offset+=l>>2;
		}

		if(!r) return 0x8000;
		os_sync_before_read(data2, len2);

return 0;
}

#if 0
// old procedure
int WBFS_direct_disc_read(u32 offset, u8 *data, u32 len)
{
int r=true;
u32 lba;
u32 len2=len;
u8* data2=data;
int l;


os_sync_after_write(data2, len2);
   
while(len>0)
	{
	lba=offset/(512);

	if(offset & 511)
		{
		//os_sync_after_write(mem_sector, 2048);
		r=USBStorage_Read_Sectors(lba, 1, mem_sector);
		if(!r) break;
			

		l=(512-(offset & 511))<<2; if(l>len) l=len;
		memcpy(data, &mem_sector[(offset & 511)<<2], l);
		os_sync_after_write(data, l);
		data+=l;
		len-=l;
		offset+=l>>2;
		
		}
	else
		{
		l=len>>11;if(l>32) l=32;
		
		if(l==0)
			{
			r=USBStorage_Read_Sectors(lba, 1, mem_sector);
			if(!r) break;
			memcpy(data, &mem_sector[0], len);
			os_sync_after_write(data, len);
			l=len;
			}
		else
            {
			r=USBStorage_Read_Sectors(lba, l, /*mem_sector*/ data);
			if(!r) break;
			os_sync_after_write(data, l<<11);
			l<<=11;
			}

		data+=l;
		len-=l;
		offset+=l>>2;
		}

	

	}

	if(!r) return 0x8000;
	os_sync_before_read(data2, len2);
return 0;
}
#endif

extern int unplug_device;

int unplug_procedure(void);

extern int is_watchdog_read_sector;

extern u32 n_sec,sec_size;

int last_sector=0;

int ehc_loop(void)
{
	ipcmessage* message;
	int timer2_id=-1;

	

	int must_read_sectors=0;
	

	
	void* queuespace = os_heap_alloc(heaphandle, 0x80);


	int queuehandle = os_message_queue_create(queuespace, 32);

	
	init_thread_ehci();

	os_thread_set_priority(os_get_thread_id(), /*os_thread_get_priority()-1*/0x78);

	os_device_register(DEVICE, queuehandle);
	timer2_id=os_create_timer(WATCHDOG_TIMER, WATCHDOG_TIMER, queuehandle, 0x0);
	
     int ums_mode = 0;
     int already_discovered = 0;
     wbfs_disc_t *d = 0;
      
	 int usb_lock=0;

	 int watch_time_on=1;


	
	
	while(1)
	{
		int result = 1;
		int ack = 1;
		
	
		// Wait for message to arrive
		os_message_queue_receive(queuehandle, (void*)&message, 0);

		

		// timer message WATCHDOG
		//if((int) message==0x555) continue;

		if(watch_time_on)
			os_stop_timer(timer2_id); // stops watchdog timer
		watch_time_on=0;
		
		is_watchdog_read_sector=0;

		if((int) message==0x0)
		{
		if(test_mode)
			watchdog_enable=0; // test mode blocks watchdog
		
		if(must_read_sectors && watchdog_enable)
			{
			int n,r;

			if(unplug_device)
				{
				for(n=0;n<3;n++)
					if(!unplug_procedure()) break;
				}

			if(unplug_device==0)
				{

				if(sec_size!=0 && sec_size<4096) // only support sector size minor to 2048
					{
					

					is_watchdog_read_sector=1;

					r=USBStorage_Read_Sectors(last_sector, 1, mem_sector);

					is_watchdog_read_sector=0;
					if(r!=0 && sec_size==512)
						last_sector+=0x1000000/sec_size; // steps of 16MB
					if(last_sector>=n_sec) last_sector=0;
					}
				
				}
			
			
			watch_time_on=1;
			os_restart_timer(timer2_id, WATCHDOG_TIMER);
			}
		continue;
		}
     

                //print_hex_dump_bytes("msg",0, message,sizeof(*message));
		switch( message->command )
		{
			case IOS_OPEN:
			{
			
                                //debug_printf("%s try open %sfor fd %d\n",DEVICE,message->open.device,message->open.resultfd);
				// Checking device name
				if (0 == strcmp(message->open.device, DEVICE))
                                  {
									result = message->open.resultfd;
                                     
										if(!already_discovered)
                                          ehci_discover();
                                        already_discovered=1;
										
                                  }
				else if (!ums_mode && 0 == memcmp(message->open.device, DEVICE"/", sizeof(DEVICE)))
                                        result = parse_and_open_device(message->open.device+sizeof(DEVICE),message->open.resultfd);
				else
					result = -6;
			}	
			break;

			case IOS_CLOSE:
			{
			
                                //debug_printf("close  fd %d\n",message->fd);
                                if(ums_mode == message->fd)
                                        ums_mode = 0;
                                else
                                        ehci_close_device(ehci_fd_to_dev(message->fd));
				// do nothing
				result = 0;
			}	
			break;

			case IOS_IOCTL:
			{
			
            break;
            }
			case IOS_IOCTLV:
			{
                                ioctlv *vec = message->ioctlv.vector;
                                void *dev =NULL;
                                int i,in = message->ioctlv.num_in,io= message->ioctlv.num_io;
                                if( 0==(message->ioctl.command>>24) && !ums_mode)
                                        dev = ehci_fd_to_dev(message->fd);
                                os_sync_before_read( vec, (in+io)*sizeof(ioctlv));
                                for(i=0;i<in+io;i++){
                                        os_sync_before_read( vec[i].data, vec[i].len);
                                        //print_hex_dump_bytes("vec",0, vec[i].data,vec[i].len);
                                }

	
                                switch( message->ioctl.command )
                                {
                                case  USB_IOCTL_CTRLMSG:
                                        //debug_printf("ctrl message%x\n",dev);
                                        if(!dev)result= -6;
                                        else
                                        result = ehci_control_message(dev,ioctlv_u8(vec[0]),ioctlv_u8(vec[1]),
                                                                      swab16(ioctlv_u16(vec[2])),swab16(ioctlv_u16(vec[3])),
                                                                      swab16(ioctlv_u16(vec[4])),ioctlv_voidp(vec[6]));
                                        break;
                                case  USB_IOCTL_BLKMSG:
                                        //debug_printf("bulk message\n");
                                        if(!dev)result= -6;
                                        else
                                                result = ehci_bulk_message(dev,ioctlv_u8(vec[0]),ioctlv_u16(vec[1]),
                                                                   ioctlv_voidp(vec[2]));
                                        break;
                                case  USB_IOCTL_INTRMSG:
                                        debug_printf("intr message\n");
                                case  USB_IOCTL_SUSPENDDEV:
                                case  USB_IOCTL_RESUMEDEV:
                                        debug_printf("or resume/suspend message\n");
                                        result = 0;//-1;// not supported
                                        break;
                                case  USB_IOCTL_GETDEVLIST:
                                        debug_printf("get dev list\n");
                                        if(dev)result= -6;
                                        else
                                        result = ehci_get_device_list(ioctlv_u8(vec[0]),ioctlv_u8(vec[1]),
                                                                      ioctlv_voidp(vec[2]),ioctlv_voidp(vec[3]));
                                        break;
                                case  USB_IOCTL_DEVREMOVALHOOK:
                                case  USB_IOCTL_DEVINSERTHOOK:
                                        debug_printf("removal/insert hook\n");
                                        ack = 0; // dont reply to those, as we dont detect anything
                                        break;
                                case USB_IOCTL_UMS_INIT: 
										must_read_sectors=0;
										
                                        result = USBStorage_Init();
										
										
										//result=-os_thread_get_priority();
										if(result>=0) {must_read_sectors=1;watchdog_enable=1;}
                                        ums_mode = message->fd;
										
                                        break;
								case USB_IOCTL_UMS_UMOUNT:
										must_read_sectors=0;
										watchdog_enable=0;
                                       // USBStorage_Umount();
										result =0;
										break;
								case USB_IOCTL_UMS_TESTMODE:
										test_mode=ioctlv_u32(vec[0]);
										result =0;
										break;
								
                                case USB_IOCTL_UMS_GET_CAPACITY:
									    n_sec =  USBStorage_Get_Capacity(&sec_size);
                                        result =n_sec ;
										if(ioctlv_voidp(vec[0]))
											{
											*((u32 *) ioctlv_voidp(vec[0]))= sec_size;
											}
										
                                        break;
                                case USB_IOCTL_UMS_READ_SECTORS:
                                       /* if (verbose)
                                                debug_printf("%p read sector %d %d %p\n",&vec[0],ioctlv_u32(vec[0]),ioctlv_u32(vec[1]), ioctlv_voidp(vec[2]));
												*/
                                        #ifdef VIGILANTE
										enable_button=1;
										#endif
										result =   USBStorage_Read_Sectors(ioctlv_u32(vec[0]),ioctlv_u32(vec[1]), ioctlv_voidp(vec[2]));
							
										//udelay(ioctlv_u32(vec[1])*125);
										if(result) break;
										
                                        break;
                                case USB_IOCTL_UMS_WRITE_SECTORS:

									    #ifdef VIGILANTE
										enable_button=1;
										#endif

                                        result =  USBStorage_Write_Sectors(ioctlv_u32(vec[0]),ioctlv_u32(vec[1]), ioctlv_voidp(vec[2]));
                                        break;
                                case USB_IOCTL_UMS_READ_STRESS:
                                      //  result =   USBStorage_Read_Stress(ioctlv_u32(vec[0]),ioctlv_u32(vec[1]), ioctlv_voidp(vec[2]));
                                        break;
                                case USB_IOCTL_UMS_SET_VERBOSE:
                                        verbose = !verbose;
                                        result =  0;
                                        break;
								/*case USB_IOCTL_WBFS_SPEED_LIMIT:
									    DVD_speed_limit=ioctlv_u32(vec[0]);
										break;*/
								case USB_IOCTL_UMS_WATCHDOG:
									    watchdog_enable=ioctlv_u32(vec[0]);
										break;
                                case USB_IOCTL_WBFS_OPEN_DISC:
                                        ums_mode = message->fd;
								        u8 *discid;
											
										int partition=0;
										#ifdef VIGILANTE
										enable_button=1;
										#endif

										discid=ioctlv_voidp(vec[0]);
										if(discid[0]=='_' && discid[1]=='D' && discid[2]=='V' && discid[3]=='D')
											{
											result = 0;watchdog_enable=1;wbfs_perform_disc();
											}
										else   
											{
											if(vec[1].len==4) memcpy(&partition, ioctlv_voidp(vec[1]), 4);
											d = wbfs_init_with_partition(discid, partition);
											if(!d)
                                               result = -1;
											else
												{result = 0;watchdog_enable=1;}
											}
                                        
										must_read_sectors=1;
										
                                        break;
								case USB_IOCTL_WBFS_STS_DISC:
									    result=USBStorage_DVD_Test();
										if(result==0) current_disc_lba=0xffffffff; // test fail
								     
										break;
								case USB_IOCTL_WBFS_READ_DIRECT_DISC: // used to read USB DVD
									    usb_lock=1;
										watchdog_enable=1;
                                        result = WBFS_direct_disc_read(ioctlv_u32(vec[0]),ioctlv_voidp(vec[2]),ioctlv_u32(vec[1]));
										usb_lock=0;
										break;
									 
                                case USB_IOCTL_WBFS_READ_DISC:
                                        /*if (verbose)
                                                debug_printf("r%x %x\n",ioctlv_u32(vec[0]),ioctlv_u32(vec[1]));
                                        else
                                                debug_printf("r%x %x\r",ioctlv_u32(vec[0]),ioctlv_u32(vec[1]));
												*/
                                        if(!d /*|| usb_lock*/)
                                                result = -1;
                                        else
									{
									
											usb_lock=1;
											//os_stop_timer(timer2_id);
                                                result = wbfs_disc_read(d,ioctlv_u32(vec[0]),ioctlv_voidp(vec[2]),ioctlv_u32(vec[1]));
											usb_lock=0;
                                       if(result){
                                          debug_printf("wbfs failed! %d\n",result);
                                          //result = 0x7800; // wii games shows unrecoverable error..
                                          result = 0x8000; 
                                        }
										//result=0;
									}
										
                                  break;
                                }
                                for(i=in;i<in+io;i++){
                                        //print_hex_dump_bytes("iovec",0, vec[i].data,vec[i].len>0x20?0x20:vec[i].len);
                                        os_sync_after_write( vec[i].data, vec[i].len);
                                }

                                break;
                        }
			default:
				result = -1;
				//ack = 0;
			break;
		}
		
        if(watchdog_enable)
			{
			watch_time_on=1;
			os_restart_timer(timer2_id, WATCHDOG_TIMER);
			}
		// Acknowledge message
		
		if (ack)
			os_message_queue_ack( (void*)message, result );

		
	}
   
	return 0;
}
