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
 ******************************************************************************/

/*
 * Alert: Not all timers of the SoC family run at a frequency of 75MHz,
 * but we should be fine as long as we make our timers an exact multiple of
 * HZ, any value that makes a 1->1 correspondence for the time conversion
 * functions to/from jiffies is acceptable.
 *
 * NOTE:CLOCK_TICK_RATE or LATCH (see include/linux/jiffies.h) should not be
 * used directly in code. Currently none of the code relevant to BERLIN
 * platform depends on these values directly.
 */
#ifndef __ASM_ARCH_TIMEX_H
#define __ASM_ARCH_TIMEX_H

#define CLOCK_TICK_RATE 75000000

#endif /* __ASM_ARCH_TIMEX_H__ */
