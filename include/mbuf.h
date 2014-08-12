/*
 * Copyright (C) 2012 Luigi Rizzo, Universita` di Pisa
 *
 * BSD copyright.
 *
 * A simple compatibility interface to map mbufs onto userspace structs
 */

#ifndef _SYS_MBUF_H_
#define	_SYS_MBUF_H_
#define	VM_UMA_H // kill this one
/* hopefully queue.h is already included by someone else */
#include <sys/queue.h>
#ifdef _KERNEL

/*
 * We implement a very simplified UMA allocator where the backend
 * is simply malloc, and uma_zone only stores the length of the components.
 */
typedef int uma_zone_t;		/* the zone size */

#define uma_zcreate(name, len, _3, _4, _5, _6, _7, _8)	(len)
typedef int (*uma_init)(void *mem, int size, int flags);
typedef void (*uma_fini)(void *mem, int size);


#define uma_zfree(zone, item)	free(item, M_IPFW)
#define uma_zalloc(zone, flags) malloc(zone, M_IPFW, flags)
#define uma_zdestroy(zone)	do {} while (0)

/*-
 * Macros for type conversion:
 * mtod(m, t)	-- Convert mbuf pointer to data pointer of correct type.
 */
#define	mtod(m, t)	((t)((m)->m_data))

#endif /* _KERNEL */

/*
 * Packet tag structure (see below for details).
 */
struct m_tag {
	SLIST_ENTRY(m_tag)	m_tag_link;	/* List of packet tags */
	u_int16_t		m_tag_id;	/* Tag ID */
	u_int16_t		m_tag_len;	/* Length of data */
	u_int32_t		m_tag_cookie;	/* ABI/Module ID */
//	void			(*m_tag_free)(struct m_tag *);
};

/*
 * Auxiliary structure to store values from the sk_buf.
 * Note that we should not alter the sk_buff, and if we do
 * so make sure to keep the values in sync between the mbuf
 * and the sk_buff (especially m_len and m_pkthdr.len).
 */

struct skbuf;

struct mbuf {
	struct mbuf *m_next;
	struct mbuf *m_nextpkt;
	void *m_data;
	int m_len;	/* length in this mbuf */
	int m_flags;
	struct {
		struct ifnet *rcvif;
		int len;	/* total packet len */
		SLIST_HEAD (packet_tags, m_tag) tags;
	} m_pkthdr;
	struct skbuf *m_skb;

	/*
	 * in-stack mbuffers point to an external buffer,
	 * the two variables below contain base and size,
	 * and have M_STACK set in m_flags.
	 * Buffers from the heap have __m_extbuf = (char *)m + MSIZE
	 */
	void *__m_extbuf;	/* external buffer base */
	int  __m_extlen;	/* data in ext buffer */
	void (*__m_callback)(struct mbuf *, int);
	void *__m_peer;		/* argument attached to the mbuf */
};

/*
 * note we also have M_FASTFWD_OURS mapped to M_PROTO1 0x10
 */
#define M_SKIP_FIREWALL	0x01		/* skip firewall processing */
#define M_BCAST         0x02 /* send/received as link-level broadcast */
#define M_MCAST         0x04 /* send/received as link-level multicast */
#define	M_PROTO1	0x10
#define	M_PROTO2	0x20
#define	M_FASTFWD_OURS	M_PROTO1
#define	M_IP_NEXTHOP	M_PROTO2
#define	M_STACK		0x1000	/* allocated on the stack */

void m_freem(struct mbuf *m);

#ifdef _KERNEL

/*
 * m_dup() is used in the TEE case, currently unsupported so we
 * just return.
 */
static __inline struct mbuf	*m_dup(struct mbuf *m, int n)
{
	(void)m;	/* UNUSED */
	(void)n;	/* UNUSED */
	D("unimplemented, expect panic");
	return NULL;
};


static __inline void
m_tag_prepend(struct mbuf *m, struct m_tag *t)
{
	ND("m %p tag %p", m, t);
	SLIST_INSERT_HEAD(&m->m_pkthdr.tags, t, m_tag_link);
}

/*
 * Unlink a tag from the list of tags associated with an mbuf.
 */
static __inline void
m_tag_unlink(struct mbuf *m, struct m_tag *t)
{

        SLIST_REMOVE(&m->m_pkthdr.tags, t, m_tag, m_tag_link);
}

/*
 * Return the next tag in the list of tags associated with an mbuf.
 */
static __inline struct m_tag *
m_tag_next(struct mbuf *m, struct m_tag *t)
{
	D("mbuf %p tag %p", m, t); 
        return (SLIST_NEXT(t, m_tag_link));
}

extern SLIST_HEAD (tags_freelist, m_tag) tags_freelist;
extern int tags_minlen;
extern int tags_freelist_count;

extern int max_protohdr;	/* uipc_mbuf.c - max proto header */

/*
 * Create an mtag of the given type
 */
static __inline struct m_tag *
m_tag_alloc(uint32_t cookie, int type, int length, int wait)
{
	static int maxlen = 0;
	int l = length + sizeof(struct m_tag);
	struct m_tag *m = NULL;

	if (l > maxlen) {
		D("new maxlen %d (%d)", l, length );
		maxlen = l;
	}
	if (l <= tags_minlen) {
		l = tags_minlen;
		m = SLIST_FIRST(&tags_freelist);
	}
	if (m) {
		SLIST_REMOVE_HEAD(&tags_freelist, m_tag_link);
		ND("allocate from freelist");
		tags_freelist_count--;
	} else {
		ND("size %d allocate from malloc", l);
		m = malloc(l, 0, M_NOWAIT);
	}
	if (m) {
		bzero(m, l);
		m->m_tag_id = type;
		m->m_tag_len = length;
		m->m_tag_cookie = cookie;
		ND("tag %p cookie %d type %d", m, cookie, type);
	}
	return m;
};

#define	MTAG_ABI_COMPAT		0		/* compatibility ABI */

static __inline struct m_tag *
m_tag_get(int type, int length, int wait)
{
	return m_tag_alloc(MTAG_ABI_COMPAT, type, length, wait);
}

static __inline struct m_tag *
m_tag_first(struct mbuf *m)
{
	struct m_tag *t;
	t = SLIST_FIRST(&m->m_pkthdr.tags);
	ND("mbuf %p has %p", m, t);
	return t;
};

static __inline void
m_tag_delete(struct mbuf *m, struct m_tag *t)
{
	D("mbuf %p tag %p, ******* unimplemented", m, t);
};

static __inline struct m_tag *
m_tag_locate(struct mbuf *m, u_int32_t cookie, int x, struct m_tag *t)
{
	struct m_tag *tag;

	ND("search %d %d in mbuf %p at %p", cookie, x, m, t);
	if (t)
		D("--- XXX ignore non-null t %p", t);
	tag = SLIST_FIRST(&m->m_pkthdr.tags);
	if (tag == NULL)
		return NULL;

	ND("found tag %p cookie %d type %d (want %d %d)",
		tag, tag->m_tag_cookie, tag->m_tag_id, cookie, x);
	if (tag->m_tag_cookie != cookie || tag->m_tag_id != x) {
		ND("want %d %d have %d %d, expect panic",
			cookie, x, tag->m_tag_cookie, tag->m_tag_id);
		return NULL;
	} else
		return tag;
};

static __inline struct m_tag *
m_tag_find(struct mbuf *m, int type, struct m_tag *start)
{
	D("m %p", m);
	return (SLIST_EMPTY(&m->m_pkthdr.tags) ? (struct m_tag *)NULL :
		m_tag_locate(m, MTAG_ABI_COMPAT, type, start));
};

#define M_SETFIB(_m, _fib)	/* nothing on linux */


/* m_pullup is not supported, there is a macro in missing.h */

#define M_GETFIB(_m)	0

/* macro used to create a new mbuf */
#define MT_DATA         1       /* dynamic (data) allocation */
#define MSIZE           256     /* size of an mbuf */
#define MGETHDR(_m, _how, _type)   ((_m) = m_gethdr((_how), (_type)))
#define MY_MCLBYTES	2048	/* XXX make slightly less */


extern struct mbuf *mbuf_freelist;

/* allocate and init a new mbuf using the same structure of FreeBSD */
/*
 * XXX for the userspace version, we actually allocate
 * MCLBYTES right after the buffer to store a copy of the packet.
 */
static __inline struct mbuf *
m_gethdr(int how, short type)
{
	struct mbuf *m;
	static const struct mbuf m0; /* zero-initialized */

	if (mbuf_freelist) {
		m = mbuf_freelist;
		mbuf_freelist = m->m_next;
		*m = m0;
	} else {
		m = malloc(MY_MCLBYTES, M_IPFW, M_NOWAIT);
	}

	ND("new mbuf %p", m);
	if (m == NULL) {
		panic("mgethdr failed");
		return m;
	}

	/* here we have MSIZE - sizeof(struct mbuf) available */
	m->m_data = m + 1;
	m->__m_extbuf = (char *)m + MSIZE;
	m->__m_extlen = MY_MCLBYTES - MSIZE;

	return m;
}


/*
 * Arrange to prepend space of size plen to mbuf m.  If a new mbuf must be
 * allocated, how specifies whether to wait.  If the allocation fails, the
 * original mbuf chain is freed and m is set to NULL.
 */
#define M_PREPEND(m, plen, how) {				\
	D("M_PREPEND not implemented");				\
}

static inline void
m_adj(struct mbuf *mp, int req_len)
{
	if (req_len < 0 || req_len > mp->m_len) {
		D("no m_adj for len %d in mlen %d", req_len, mp->m_len);
	} else {
		mp->m_data += req_len;
		mp->m_len += req_len;
	}
}

#define M_PREPEND_GOOD(m, plen, how) do {                                    \
        struct mbuf **_mmp = &(m);                                      \
        struct mbuf *_mm = *_mmp;                                       \
        int _mplen = (plen);                                            \
        int __mhow = (how);                                             \
                                                                        \
        MBUF_CHECKSLEEP(how);                                           \
        if (M_LEADINGSPACE(_mm) >= _mplen) {                            \
                _mm->m_data -= _mplen;                                  \
                _mm->m_len += _mplen;                                   \
        } else                                                          \
                _mm = m_prepend(_mm, _mplen, __mhow);                   \
        if (_mm != NULL && _mm->m_flags & M_PKTHDR)                     \
                _mm->m_pkthdr.len += _mplen;                            \
        *_mmp = _mm;                                                    \
} while (0)

/*
 * Persistent tags stay with an mbuf until the mbuf is reclaimed.  Otherwise
 * tags are expected to ``vanish'' when they pass through a network
 * interface.  For most interfaces this happens normally as the tags are
 * reclaimed when the mbuf is free'd.  However in some special cases
 * reclaiming must be done manually.  An example is packets that pass through
 * the loopback interface.  Also, one must be careful to do this when
 * ``turning around'' packets (e.g., icmp_reflect).
 *
 * To mark a tag persistent bit-or this flag in when defining the tag id.
 * The tag will then be treated as described above.
 */
#define	MTAG_PERSISTENT				0x800

#define	PACKET_TAG_NONE				0  /* Nadda */

/* Packet tags for use with PACKET_ABI_COMPAT. */
#define	PACKET_TAG_IPSEC_IN_DONE		1  /* IPsec applied, in */
#define	PACKET_TAG_IPSEC_OUT_DONE		2  /* IPsec applied, out */
#define	PACKET_TAG_IPSEC_IN_CRYPTO_DONE		3  /* NIC IPsec crypto done */
#define	PACKET_TAG_IPSEC_OUT_CRYPTO_NEEDED	4  /* NIC IPsec crypto req'ed */
#define	PACKET_TAG_IPSEC_IN_COULD_DO_CRYPTO	5  /* NIC notifies IPsec */
#define	PACKET_TAG_IPSEC_PENDING_TDB		6  /* Reminder to do IPsec */
#define	PACKET_TAG_BRIDGE			7  /* Bridge processing done */
#define	PACKET_TAG_GIF				8  /* GIF processing done */
#define	PACKET_TAG_GRE				9  /* GRE processing done */
#define	PACKET_TAG_IN_PACKET_CHECKSUM		10 /* NIC checksumming done */
#define	PACKET_TAG_ENCAP			11 /* Encap.  processing */
#define	PACKET_TAG_IPSEC_SOCKET			12 /* IPSEC socket ref */
#define	PACKET_TAG_IPSEC_HISTORY		13 /* IPSEC history */
#define	PACKET_TAG_IPV6_INPUT			14 /* IPV6 input processing */
#define	PACKET_TAG_DUMMYNET			15 /* dummynet info */
#define	PACKET_TAG_DIVERT			17 /* divert info */
#define	PACKET_TAG_IPFORWARD			18 /* ipforward info */
#define	PACKET_TAG_MACLABEL	(19 | MTAG_PERSISTENT) /* MAC label */
#define	PACKET_TAG_PF				21 /* PF + ALTQ information */
#define	PACKET_TAG_RTSOCKFAM			25 /* rtsock sa family */
#define	PACKET_TAG_IPOPTIONS			27 /* Saved IP options */
#define	PACKET_TAG_CARP                         28 /* CARP info */

#endif /* _KERNEL */
#endif /* !_SYS_MBUF_H_ */
