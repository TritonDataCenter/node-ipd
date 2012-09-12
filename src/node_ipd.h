#ifndef _NODE_IPD_H
#define	_NODE_IPD_H

#include <sys/types.h>
#include <libipd.h>
#include <libnvpair.h>
#include "v8plus_glue.h"

#ifdef	__cplusplus
extern "C" {
#endif	/* __cplusplus */

#define	__UNUSED	__attribute__((__unused__))

typedef struct node_ipd {
	int ni_fd;
	zoneid_t ni_zid;
} node_ipd_t;

#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif	/* _NODE_CONTRACT_H */
