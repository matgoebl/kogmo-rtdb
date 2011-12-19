/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */

#ifndef KOGMO_RTDB_OBJDATA_H
#define KOGMO_RTDB_OBJDATA_H

extern const volatile kogmo_timestamp_t invalid_ts;

// t_poll = FACTOR * avg_cycletime
#define KOGMO_RTDB_NONOTIFIES_POLLTIME_FACTOR (0.1)
// t_poll is limited to MAX seconds
#define KOGMO_RTDB_NONOTIFIES_POLLTIME_MAX (0.1)

#endif /* KOGMO_RTDB_OBJDATA_H */
