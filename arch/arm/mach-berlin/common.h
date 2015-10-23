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

struct sys_timer;
extern struct sys_timer apb_timer;

extern struct smp_operations berlin_smp_ops;
extern void __init berlin_init_irq(void);
extern void berlin_cpu_die(unsigned int cpu);
extern void __init berlin_clk_init(void);
#ifdef CONFIG_BERLIN2CD
extern void __init berlin_map_io(void);
extern void __init berlin2cd_reset(void);
#endif
