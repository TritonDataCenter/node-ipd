/*
 * Copyright (c) 2012, Joyent, Inc.  All rights reserved.
 */

#include <sys/ccompile.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <libnvpair.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <libipd.h>
#include <zone.h>
#include "node_ipd.h"

static int
close_on_exec(int fd)
{
	int flags;
	int err;

	if ((flags = fcntl(fd, F_GETFD, 0)) < 0 ||
	    fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) {
		err = errno;
		(void) v8plus_syserr(err, "unable to set CLOEXEC: %s",
		    strerror(err));
		return (-1);
	}

	return (0);
}

static nvlist_t *
ipd_error(void)
{
	v8plus_errno_t e;

	switch (ipd_errno) {
	case EIPD_NOERROR:
		e = V8PLUSERR_NOERROR;
		break;
	case EIPD_NOMEM:
		e = V8PLUSERR_NOMEM;
		break;
	case EIPD_ZC_NOENT:
		e = V8PLUSERR_ZC_NOENT;
		break;
	case EIPD_RANGE:
		e = V8PLUSERR_RANGE;
		break;
	case EIPD_PERM:
		e = V8PLUSERR_PERM;
		break;
	case EIPD_FAULT:
		v8plus_panic("unexpected EFAULT: %s", ipd_errmsg);
		break;
	case EIPD_INTERNAL:
	default:
		e = V8PLUSERR_IPD;
		break;
	}

	return (v8plus_error(e, "libipd error: %s", ipd_errmsg));
}

static void
node_ipd_free(node_ipd_t *ip)
{
	if (ip == NULL)
		return;

	if (ip->ni_fd != -1)
		(void) ipd_close(ip->ni_fd);

	free(ip);
}

static nvlist_t *
node_ipd_ctor(const nvlist_t *ap, void **ipp)
{
	int fd;
	node_ipd_t *nip;
	double d;
	char *s;
	zoneid_t zid;

	if (v8plus_args(ap, V8PLUS_ARG_F_NOEXTRA,
	    V8PLUS_TYPE_NUMBER, &d,
	    V8PLUS_TYPE_NONE) == 0) {
		zid = (zoneid_t)d;
	} else if (v8plus_args(ap, V8PLUS_ARG_F_NOEXTRA,
	    V8PLUS_TYPE_STRING, &s,
	    V8PLUS_TYPE_NONE) == 0) {
		zid = getzoneidbyname(s);
		if (zid < 0) {
			return (v8plus_error(V8PLUSERR_ZC_NOENT,
			    "no such zone %s", s));
		}
	} else if (v8plus_args(ap, V8PLUS_ARG_F_NOEXTRA,
	    V8PLUS_TYPE_NONE) == 0 ||
	    v8plus_args(ap, V8PLUS_ARG_F_NOEXTRA,
	    V8PLUS_TYPE_UNDEFINED, V8PLUS_TYPE_NONE) == 0) {
		zid = getzoneid();
	} else {
		return (NULL);
	}

	if ((fd = ipd_open(NULL)) < 0)
		return (ipd_error());

	if (close_on_exec(fd) != 0) {
		(void) close(fd);
		return (NULL);
	}

	if ((nip = malloc(sizeof (node_ipd_t))) == NULL) {
		(void) ipd_close(fd);
		return (v8plus_syserr(errno, "unable to allocate ipd handle"));
	}

	nip->ni_fd = fd;
	nip->ni_zid = zid;

	*ipp = nip;
	return (v8plus_void());
}

static void
node_ipd_dtor(void *op)
{
	node_ipd_t *nip = op;

	node_ipd_free(nip);
}

static int
ipd_status_prop(nvlist_t *rp, const ipd_config_t *icp, uint32_t mask)
{
	const char *key;
	uint32_t val;

	switch (mask) {
	case IPDM_CORRUPT:
		key = "corrupt";
		val = icp->ic_corrupt;
		break;
	case IPDM_DELAY:
		key = "delay";
		val = icp->ic_delay;
		break;
	case IPDM_DROP:
		key = "drop";
		val = icp->ic_drop;
		break;
	default:
		v8plus_panic("unknown ipd attribute %u", mask);
		break;
	}

	if (!(icp->ic_mask & mask))
		return (0);

	if (v8plus_obj_setprops(rp,
	    V8PLUS_TYPE_NUMBER, key, (double)val,
	    V8PLUS_TYPE_NONE) != 0) {
		nvlist_free(rp);
		return (-1);
	}

	return (0);
}

static nvlist_t *
node_ipd_getDisturb(void *op, const nvlist_t *ap __UNUSED)
{
	node_ipd_t *nip = op;
	nvlist_t *lp, *rp;
	ipd_stathdl_t is;
	ipd_config_t *icp;
	int err;

	if (ipd_status_read(nip->ni_fd, &is) != 0)
		return (ipd_error());

	if (ipd_status_get_config(is, nip->ni_zid, &icp) != 0) {
		(void) ipd_error();
		ipd_status_free(is);
		return (NULL);
	}

	if ((err = nvlist_alloc(&lp, NV_UNIQUE_NAME, 0) != 0)) {
		ipd_status_free(is);
		return (v8plus_nverr(err, NULL));
	}

	if (ipd_status_prop(lp, icp, IPDM_CORRUPT) != 0 ||
	    ipd_status_prop(lp, icp, IPDM_DELAY) != 0 ||
	    ipd_status_prop(lp, icp, IPDM_DROP) != 0) {
		ipd_status_free(is);
		return (NULL);
	}

	ipd_status_free(is);
	rp = v8plus_obj(V8PLUS_TYPE_OBJECT, "res", lp, V8PLUS_TYPE_NONE);
	nvlist_free(lp);
	return (rp);
}

static nvlist_t *
node_ipd_setDisturb(void *op, const nvlist_t *ap)
{
	node_ipd_t *nip = op;
	ipd_config_t ic;
	double d;
	nvlist_t *aop;

	bzero(&ic, sizeof (ic));

	if (v8plus_args(ap, V8PLUS_ARG_F_NOEXTRA, V8PLUS_TYPE_NONE) == 0)
		return (v8plus_void());

	if (v8plus_args(ap, V8PLUS_ARG_F_NOEXTRA,
	    V8PLUS_TYPE_OBJECT, &aop, V8PLUS_TYPE_NONE) != 0)
		return (NULL);

	if (nvlist_lookup_double(aop, "corrupt", &d) == 0) {
		ic.ic_mask |= IPDM_CORRUPT;
		ic.ic_corrupt = (uint32_t)d;
	}
	if (nvlist_lookup_double(aop, "delay", &d) == 0) {
		ic.ic_mask |= IPDM_DELAY;
		ic.ic_delay = (uint32_t)d;
	}
	if (nvlist_lookup_double(aop, "drop", &d) == 0) {
		ic.ic_mask |= IPDM_DROP;
		ic.ic_drop = (uint32_t)d;
	}

	if (ipd_ctl(nip->ni_fd, nip->ni_zid, &ic) != 0)
		return (ipd_error());

	return (v8plus_void());
}

/*
 * v8+ boilerplate
 */
const v8plus_c_ctor_f v8plus_ctor = node_ipd_ctor;
const v8plus_c_dtor_f v8plus_dtor = node_ipd_dtor;
const char *v8plus_js_factory_name = "create";
const char *v8plus_js_class_name = "IPDBinding";
const v8plus_method_descr_t v8plus_methods[] = {
	{
		md_name: "getDisturb",
		md_c_func: node_ipd_getDisturb
	},
	{
		md_name: "setDisturb",
		md_c_func: node_ipd_setDisturb
	}
};
const uint_t v8plus_method_count =
    sizeof (v8plus_methods) / sizeof (v8plus_methods[0]);

const v8plus_static_descr_t v8plus_static_methods[] = {};
const uint_t v8plus_static_method_count = 0;
