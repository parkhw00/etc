#ifndef _LINK_H_
#define _LINK_H_

#include <stddef.h>

struct link
{
	struct link *prev;
	struct link *next;
};

static inline void link_del (struct link *del)
{
	del->prev->next = del->next;
	del->next->prev = del->prev;
}

static inline void link_add (struct link *root, struct link *add)
{
	root->next->prev = add;
	add->next = root->next;
	root->next = add;
	add->prev = root;
}

static inline void link_add_tail (struct link *root, struct link *add)
{
	root->prev->next = add;
	add->prev = root->prev;
	root->prev = add;
	add->next = root;
}

#define DEFINE_LINK(name)	struct link name = {&name, &name}
#define link_init(link)		({ \
	(link)->next = (link); \
	(link)->prev = (link); \
	0; })

#define link_container(link, type, member) \
	(type*)(((void*)(link)) - offsetof (type, member))

#define link_for_each(root, now, member) \
	for (now=link_container((root)->next, typeof (*(now)), member); \
		&(now)->member != (root); \
		now = link_container ((now)->member.next, typeof (*(now)), member))

#endif
