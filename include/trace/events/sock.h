#undef TRACE_SYSTEM
#define TRACE_SYSTEM sock

#if !defined(_TRACE_SOCK_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SOCK_H

#include <net/sock.h>
#include <net/ipv6.h>
#include <linux/tracepoint.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>

/* The protocol traced by sock_set_state */
#define inet_protocol_names		\
		EM(IPPROTO_TCP)			\
		EM(IPPROTO_DCCP)		\
		EMe(IPPROTO_SCTP)

#define tcp_state_names			\
		EM(TCP_ESTABLISHED)		\
		EM(TCP_SYN_SENT)		\
		EM(TCP_SYN_RECV)		\
		EM(TCP_FIN_WAIT1)		\
		EM(TCP_FIN_WAIT2)		\
		EM(TCP_TIME_WAIT)		\
		EM(TCP_CLOSE)			\
		EM(TCP_CLOSE_WAIT)		\
		EM(TCP_LAST_ACK)		\
		EM(TCP_LISTEN)			\
		EM(TCP_CLOSING)			\
		EMe(TCP_NEW_SYN_RECV)

/* enums need to be exported to user space */
#undef EM
#undef EMe
#define EM(a)       TRACE_DEFINE_ENUM(a);
#define EMe(a)      TRACE_DEFINE_ENUM(a);

inet_protocol_names
tcp_state_names

#undef EM
#undef EMe
#define EM(a)       { a, #a },
#define EMe(a)      { a, #a }

#define show_inet_protocol_name(val)    \
	__print_symbolic(val, inet_protocol_names)

#define show_tcp_state_name(val)        \
	__print_symbolic(val, tcp_state_names)

TRACE_EVENT(sock_rcvqueue_full,

	TP_PROTO(struct sock *sk, struct sk_buff *skb),

	TP_ARGS(sk, skb),

	TP_STRUCT__entry(
		__field(int, rmem_alloc)
		__field(unsigned int, truesize)
		__field(int, sk_rcvbuf)
	),

	TP_fast_assign(
		__entry->rmem_alloc = atomic_read(&sk->sk_rmem_alloc);
		__entry->truesize   = skb->truesize;
		__entry->sk_rcvbuf  = sk->sk_rcvbuf;
	),

	TP_printk("rmem_alloc=%d truesize=%u sk_rcvbuf=%d",
		__entry->rmem_alloc, __entry->truesize, __entry->sk_rcvbuf)
);

TRACE_EVENT(sock_exceed_buf_limit,

	TP_PROTO(struct sock *sk, struct proto *prot, long allocated),

	TP_ARGS(sk, prot, allocated),

	TP_STRUCT__entry(
		__array(char, name, 32)
		__array(long, sysctl_mem, 3)
		__field(long, allocated)
		__field(int, sysctl_rmem)
		__field(int, rmem_alloc)
	),

	TP_fast_assign(
		strncpy(__entry->name, prot->name, 32);
		__entry->sysctl_mem[0] = READ_ONCE(prot->sysctl_mem[0]);
		__entry->sysctl_mem[1] = READ_ONCE(prot->sysctl_mem[1]);
		__entry->sysctl_mem[2] = READ_ONCE(prot->sysctl_mem[2]);
		__entry->allocated = allocated;
		__entry->sysctl_rmem = prot->sysctl_rmem[0];
		__entry->rmem_alloc = atomic_read(&sk->sk_rmem_alloc);
	),

	TP_printk("proto:%s sysctl_mem=%ld,%ld,%ld allocated=%ld "
		"sysctl_rmem=%d rmem_alloc=%d",
		__entry->name,
		__entry->sysctl_mem[0],
		__entry->sysctl_mem[1],
		__entry->sysctl_mem[2],
		__entry->allocated,
		__entry->sysctl_rmem,
		__entry->rmem_alloc)
);

TRACE_EVENT(inet_sock_set_state,

	TP_PROTO(const struct sock *sk, const int oldstate, const int newstate),

	TP_ARGS(sk, oldstate, newstate),

	TP_STRUCT__entry(
		__field(const void *, skaddr)
		__field(int, oldstate)
		__field(int, newstate)
		__field(__u16, sport)
		__field(__u16, dport)
		__field(__u8, protocol)
		__array(__u8, saddr, 4)
		__array(__u8, daddr, 4)
		__array(__u8, saddr_v6, 16)
		__array(__u8, daddr_v6, 16)
	),

	TP_fast_assign(
		struct inet_sock *inet = inet_sk(sk);
		struct in6_addr *pin6;
		__be32 *p32;

		__entry->skaddr = sk;
		__entry->oldstate = oldstate;
		__entry->newstate = newstate;

		__entry->protocol = sk->sk_protocol;
		__entry->sport = ntohs(inet->inet_sport);
		__entry->dport = ntohs(inet->inet_dport);

		p32 = (__be32 *) __entry->saddr;
		*p32 = inet->inet_saddr;

		p32 = (__be32 *) __entry->daddr;
		*p32 =  inet->inet_daddr;

#if IS_ENABLED(CONFIG_IPV6)
		if (sk->sk_family == AF_INET6) {
			pin6 = (struct in6_addr *)__entry->saddr_v6;
			*pin6 = sk->sk_v6_rcv_saddr;
			pin6 = (struct in6_addr *)__entry->daddr_v6;
			*pin6 = sk->sk_v6_daddr;
		} else
#endif
		{
			pin6 = (struct in6_addr *)__entry->saddr_v6;
			ipv6_addr_set_v4mapped(inet->inet_saddr, pin6);
			pin6 = (struct in6_addr *)__entry->daddr_v6;
			ipv6_addr_set_v4mapped(inet->inet_daddr, pin6);
		}
	),

	TP_printk("protocol=%s sport=%hu dport=%hu saddr=%pI4 daddr=%pI4"
			"saddrv6=%pI6c daddrv6=%pI6c oldstate=%s newstate=%s",
			show_inet_protocol_name(__entry->protocol),
			__entry->sport, __entry->dport,
			__entry->saddr, __entry->daddr,
			__entry->saddr_v6, __entry->daddr_v6,
			show_tcp_state_name(__entry->oldstate),
			show_tcp_state_name(__entry->newstate))
);

#endif /* _TRACE_SOCK_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
