/* tcp_out.c  -  tpc_out */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  tcp_out  -  TCP output process that executes scheduled events
 *------------------------------------------------------------------------
 */
process	tcp_out(void)
{
	struct	tcb	*tcbptr;	/* Ptr to a TCB			*/
	int32	msg;			/* Message from a message queue	*/

	/* Continually wait for a message and perform the command */

	while (1) {

		/* Extract next message */
		msg = mqrecv (Tcp.tcpcmdq);

		/* Extract the TCB pointer from the message */
		tcbptr = TCBCMD_TCB(msg);
                
                
		/* Obtain exclusive use of TCP */
		wait (Tcp.tcpmutex);

		/* Obtain exclusive use of the TCB */
		wait (tcbptr->tcb_mutex);

		/* Insure TCB has remained active */
		if (tcbptr->tcb_state <= TCB_CLOSED) {
			tcbunref (tcbptr);
			signal (tcbptr->tcb_mutex);
			signal (Tcp.tcpmutex);
			continue;
		}
		signal (Tcp.tcpmutex);

		/* Perform the command */

		switch (TCBCMD_CMD(msg)) {

		/* Send data */

		case TCBC_SEND:

			tcpxmit (tcbptr, tcbptr->tcb_snext);
			 
			break;

		/* Send a delayed ACK */

		case TCBC_DELACK:
			tcpack (tcbptr, FALSE);
			break;

		/* Retransmission Timer expired */

		case TCBC_RTO:
			tcbptr->tcb_ssthresh = max(tcbptr->tcb_cwnd >> 1,
						 tcbptr->tcb_mss);
			tcbptr->tcb_cwnd = tcbptr->tcb_mss;
			tcbptr->tcb_dupacks = 0;
			tcbptr->tcb_rto <<= 1;
			tcbptr ->tcb_rtocount++;
			
			if (tcbptr->tcb_rtocount > TCP_MAXRTO){ //++tcbptr->tcb_rtocount > TCP_MAXRTO
				tcpabort (tcbptr);
			}else{
				tcpxmit (tcbptr, tcbptr->tcb_suna);
			}
			break;

		/* TCB has expired, so mark it closed */

		case TCBC_EXPIRE:
			tcbptr->tcb_state = TCB_CLOSED;
			//tcbunref (tcbptr); 
			break;

		/* Unknown command (should not happen) */

		default:
			break;
		}
		/* Command has been handled, so reduce reference count	*/

		tcbunref (tcbptr);

		/* Release TCP mutex while waiting for next message	*/

		signal (tcbptr->tcb_mutex);
	}

	/* Will never reach this point */

	return SYSERR;
}
