/*
 * netifd - network interface daemon
 * Copyright (C) 2012 Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <string.h>
#include <stdlib.h>
#include "utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>

void
__vlist_simple_init(struct vlist_simple_tree *tree, int offset)
{
	INIT_LIST_HEAD(&tree->list);
	tree->version = 1;
	tree->head_offset = offset;
}

void
vlist_simple_delete(struct vlist_simple_tree *tree, struct vlist_simple_node *node)
{
	char *ptr;

	list_del(&node->list);
	ptr = (char *) node - tree->head_offset;
	free(ptr);
}

void
vlist_simple_flush(struct vlist_simple_tree *tree)
{
	struct vlist_simple_node *n, *tmp;

	list_for_each_entry_safe(n, tmp, &tree->list, list) {
		if ((n->version == tree->version || n->version == -1) &&
		    tree->version != -1)
			continue;

		vlist_simple_delete(tree, n);
	}
}

void
vlist_simple_replace(struct vlist_simple_tree *dest, struct vlist_simple_tree *old)
{
	struct vlist_simple_node *n, *tmp;

	vlist_simple_update(dest);
	list_for_each_entry_safe(n, tmp, &old->list, list) {
		list_del(&n->list);
		vlist_simple_add(dest, n);
	}
	vlist_simple_flush(dest);
}

void
vlist_simple_flush_all(struct vlist_simple_tree *tree)
{
	tree->version = -1;
	vlist_simple_flush(tree);
}

unsigned int
parse_netmask_string(const char *str, bool v6)
{
	struct in_addr addr;
	unsigned int ret;
	char *err = NULL;

	if (!strchr(str, '.')) {
		ret = strtoul(str, &err, 0);
		if (err && *err)
			goto error;

		return ret;
	}

	if (v6)
		goto error;

	if (inet_aton(str, &addr) != 1)
		goto error;

	return 32 - fls(~(ntohl(addr.s_addr)));

error:
	return ~0;
}

bool
split_netmask(char *str, unsigned int *netmask, bool v6)
{
	char *delim = strchr(str, '/');

	if (delim) {
		*(delim++) = 0;

		*netmask = parse_netmask_string(delim, v6);
	}
	return true;
}

int
parse_ip_and_netmask(int af, const char *str, void *addr, unsigned int *netmask)
{
	char *astr = alloca(strlen(str) + 1);

	strcpy(astr, str);
	if (!split_netmask(astr, netmask, af == AF_INET6))
		return 0;

	if (af == AF_INET6) {
		if (*netmask > 128)
			return 0;
	} else {
		if (*netmask > 32)
			return 0;
	}

	return inet_pton(af, astr, addr);
}
