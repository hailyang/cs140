#include "vm/spage.h"
#include <hash.h>
#include <stdlib.h>
#include <stdio.h>
#include <debug.h>

static bool spage_func_less (const struct hash_elem *a, 
	const struct hash_elem *b, void *aux UNUSED);
static unsigned spage_func_hash (const struct hash_elem *p, void *aux UNUSED);

void
spage_init (struct hash *spage_hash)
{
  hash_init (spage_hash, spage_func_hash, spage_func_less, NULL);
}

struct spage_entry *
spage_lookup (struct hash *spage_hash, void *uaddr)
{
  struct spage_entry s;
  struct hash_elem *e;
  s.uaddr = uaddr;
  e = hash_find (spage_hash, &s.elem);
  if (e != NULL)
    return hash_entry (e, struct spage_entry, elem);
  return NULL;
}

static bool
spage_func_less (const struct hash_elem *a,
        const struct hash_elem *b, void *aux UNUSED)
{
   const struct spage_entry *sa =
	hash_entry (a, struct spage_entry, elem);
   const struct spage_entry *sb =
	hash_entry (b, struct spage_entry, elem);
   return sa->uaddr < sb->uaddr;
}

static unsigned 
spage_func_hash (const struct hash_elem *p, void *aux UNUSED) 
{
  const struct spage_entry *s =
	hash_entry (p, struct spage_entry, elem);
  return hash_bytes (&s->uaddr, sizeof s->uaddr);
}
