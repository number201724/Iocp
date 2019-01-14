#pragma once

typedef struct _MP_LIST_ENTRY
{
	struct _MP_LIST_ENTRY *prev, *next;
}MP_LIST_ENTRY, *PMP_LIST_ENTRY;



static inline void InitializeMPListEntry( struct _MP_LIST_ENTRY *list )
{
	list->prev = list->next = list;
}

static inline void __MP_LIST_ADD( struct _MP_LIST_ENTRY *entry,
								  struct _MP_LIST_ENTRY *prev,
								  struct _MP_LIST_ENTRY *next )
{
	next->prev = entry;
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
}

static inline void MPListInsertToTail( struct _MP_LIST_ENTRY *head,
						 struct _MP_LIST_ENTRY *entry )
{
	__MP_LIST_ADD( entry, head->prev, head );
}

static inline void MPListInsertToHead( struct _MP_LIST_ENTRY *head,
						 struct _MP_LIST_ENTRY *entry )
{
	__MP_LIST_ADD( entry, head, head->next );
}

static inline void __MP_LIST_DEL( struct _MP_LIST_ENTRY * prev,
								  struct _MP_LIST_ENTRY * next )
{
	next->prev = prev;
	prev->next = next;
}

static inline void MPRemoveEntryList( struct _MP_LIST_ENTRY *entry )
{
	__MP_LIST_DEL( entry->prev, entry->next );
	entry->next = NULL;
	entry->prev = NULL;
}

static inline bool MPListIsEmpty( const struct _MP_LIST_ENTRY *head )
{
	return head->next == head;
}

static inline int MPListIsLast( const struct _MP_LIST_ENTRY *list,
				  const struct _MP_LIST_ENTRY *head )
{
	return list->next == head;
}

static inline int MPListIsFirst( const struct _MP_LIST_ENTRY *list,
				   const struct _MP_LIST_ENTRY *head )
{
	return list->prev == head;
}

#ifndef container_of
#define container_of(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))
#endif

//linux replace to container_of
#define MPListFirstEntry(H,S,M) container_of((H)->next,S,M)
#define MPListLastEntry(H,S,M) container_of((H)->prev,S,M)