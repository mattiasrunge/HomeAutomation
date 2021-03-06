
#include "sns_rfTransceive.h"

StdCan_Msg_t		rfTxMsg;

#if IR_RX_ENABLE==1
uint8_t rfRxChannel_newData;
uint8_t rfRxChannel_rxlen;
uint8_t rfRxChannel_state;
Ir_Protocol_Data_t	rfRxChannel_proto;
uint16_t	rfRxChannel_buf[MAX_NR_TIMES];

void sns_rfTransceive_RX_done_callback(uint8_t channel, uint16_t *buffer, uint8_t len)
{
	rfRxChannel_newData = TRUE;
	rfRxChannel_rxlen = len;
}
#endif


#if IR_TX_ENABLE==1
uint8_t rfTxChannel_sendComplete;
uint8_t rfTxChannel_state;
uint8_t rfTxChannel_txlen;
Ir_Protocol_Data_t	rfTxChannel_proto;
uint16_t	rfTxChannel_buf[MAX_NR_TIMES];
uint8_t rfTxChannel_repeatCount;
uint8_t rfTxChannel_stopSend;

void sns_rfTransceive_TX_done_callback(uint8_t channel)
{
	rfTxChannel_sendComplete = TRUE;
}
#endif

void sns_rfTransceive_Init(void)
{
	StdCan_Set_class(rfTxMsg.Header, CAN_MODULE_CLASS_SNS);
	StdCan_Set_direction(rfTxMsg.Header, DIRECTIONFLAG_FROM_OWNER);
	rfTxMsg.Header.ModuleType = CAN_MODULE_TYPE_SNS_RFTRANSCEIVE;
	rfTxMsg.Header.ModuleId = sns_rfTransceive_ID;
	rfTxMsg.Length = 8;
	
#if IR_RX_ENABLE==1
	rfRxChannel_newData = FALSE;
	rfRxChannel_rxlen = 0;
	rfRxChannel_proto.timeout=0;
	rfRxChannel_proto.data=0;
	rfRxChannel_proto.repeats=0;
	rfRxChannel_proto.protocol=0;
#endif

#if IR_TX_ENABLE==1
	rfTxChannel_sendComplete = FALSE;
	rfTxChannel_txlen = 0;
	rfTxChannel_proto.data=0;
	rfTxChannel_proto.repeats=0;
	rfTxChannel_proto.framecnt=0;
	rfTxChannel_proto.protocol=0;
	rfTxChannel_repeatCount = 0;
#endif

	IrTransceiver_Init();
	/* TX-pin must be set in case transmitter is nexa */
	gpio_set_out(sns_rfTransceive_TX_PIN);
#if IR_TX_ACTIVE_LOW==1
	gpio_set_pin(sns_rfTransceive_TX_PIN);
#else
	gpio_clr_pin(sns_rfTransceive_TX_PIN);
#endif

#if IR_RX_ENABLE==1
	IrTransceiver_InitRxChannel(0, rfRxChannel_buf, sns_rfTransceive_RX_done_callback, sns_rfTransceive_RX_PCINT, sns_rfTransceive_RX_PIN);
	rfRxChannel_state = sns_rfTransceive_STATE_RECEIVING;
#endif
	
#if IR_TX_ENABLE==1
	IrTransceiver_InitTxChannel(0, sns_rfTransceive_TX_done_callback, sns_rfTransceive_TX_PIN);
	rfTxChannel_state = sns_rfTransceive_STATE_IDLE;
#endif
}

void sns_rfTransceive_Process(void)
{
#if IR_RX_ENABLE==1
		switch (rfRxChannel_state)
		{
		case sns_rfTransceive_STATE_IDLE:
		{
			/* If known protocol and timeout is not 0 (0 means burst) */
			if (rfRxChannel_proto.protocol != IR_PROTO_UNKNOWN && rfRxChannel_proto.timeout != 0) {
				/* Send button release command on CAN */
				rfTxMsg.Header.Command = CAN_MODULE_CMD_RFTRANSCEIVE_DATASTOP;
				rfTxMsg.Data[0] = 0;
				rfTxMsg.Data[1] = rfRxChannel_proto.protocol;
				rfTxMsg.Data[2] = (rfRxChannel_proto.data>>40)&0xff;
				rfTxMsg.Data[3] = (rfRxChannel_proto.data>>32)&0xff;
				rfTxMsg.Data[4] = (rfRxChannel_proto.data>>24)&0xff;
				rfTxMsg.Data[5] = (rfRxChannel_proto.data>>16)&0xff;
				rfTxMsg.Data[6] = (rfRxChannel_proto.data>>8)&0xff;
				rfTxMsg.Data[7] = rfRxChannel_proto.data&0xff;

				StdCan_Put(&rfTxMsg);
			}
			rfRxChannel_state = sns_rfTransceive_STATE_START_RECEIVE;
			break;
		}

		case sns_rfTransceive_STATE_START_RECEIVE:
			cli();
			rfRxChannel_newData = FALSE;
			sei();
			rfRxChannel_state = sns_rfTransceive_STATE_RECEIVING;
			
			break;

		case sns_rfTransceive_STATE_RECEIVING:
			if (rfRxChannel_newData == TRUE) {
				cli();
				rfRxChannel_newData = FALSE;
				sei();
				
				/* Let protocol driver parse and then send on CAN */
				uint8_t res2 = parseProtocol(rfRxChannel_buf, rfRxChannel_rxlen, &rfRxChannel_proto);
				if (res2 == IR_OK && rfRxChannel_proto.protocol != IR_PROTO_UNKNOWN) {
					/* If timeout is 0, protocol is burst protocol */
					if (rfRxChannel_proto.timeout > 0)
					{
						rfTxMsg.Header.Command = CAN_MODULE_CMD_RFTRANSCEIVE_DATASTART;
						rfRxChannel_state = sns_rfTransceive_STATE_START_PAUSE;
					}
					else
					{
						rfTxMsg.Header.Command = CAN_MODULE_CMD_RFTRANSCEIVE_DATABURST;
						rfRxChannel_state = sns_rfTransceive_STATE_START_RECEIVE;
					}
					rfTxMsg.Data[0] = 0;
					rfTxMsg.Data[1] = rfRxChannel_proto.protocol;
					rfTxMsg.Data[2] = (rfRxChannel_proto.data>>40)&0xff;
					rfTxMsg.Data[3] = (rfRxChannel_proto.data>>32)&0xff;
					rfTxMsg.Data[4] = (rfRxChannel_proto.data>>24)&0xff;
					rfTxMsg.Data[5] = (rfRxChannel_proto.data>>16)&0xff;
					rfTxMsg.Data[6] = (rfRxChannel_proto.data>>8)&0xff;
					rfTxMsg.Data[7] = rfRxChannel_proto.data&0xff;

					StdCan_Put(&rfTxMsg);
				}
				else if (rfRxChannel_proto.protocol == IR_PROTO_UNKNOWN)
				{
#if (sns_rfTransceive_SEND_DEBUG==1)
					//send_debug(rfRxChannel_buf, rfRxChannel_rxlen);
					//rfRxChannel_proto.timeout=300;
#endif
					rfRxChannel_state = sns_rfTransceive_STATE_START_RECEIVE;
				}
			}
			break;

		case sns_rfTransceive_STATE_START_PAUSE:
			/* set a timer so we can send release button event when no new RF is arriving */
			Timer_SetTimeout(sns_rfTransceive_RX_REPEATE_TIMER, rfRxChannel_proto.timeout, TimerTypeOneShot, 0);
			rfRxChannel_state = sns_rfTransceive_STATE_PAUSING;
			break;

		case sns_rfTransceive_STATE_PAUSING:
			/* reset timer if new IR arrived */
			if (rfRxChannel_newData == TRUE) {
				cli();
				IrTransceiver_ResetRx(0); //TODO??
				rfRxChannel_newData = FALSE;
				sei();

				Ir_Protocol_Data_t	protoDummy;
				if (parseProtocol(rfRxChannel_buf, rfRxChannel_rxlen, &protoDummy) == IR_OK) {
					if (protoDummy.protocol == rfRxChannel_proto.protocol) {
						/* re-set timer so we can send release button event when no new RF is arriving */
						Timer_SetTimeout(sns_rfTransceive_RX_REPEATE_TIMER, rfRxChannel_proto.timeout, TimerTypeOneShot, 0);
					}
				}
			}

			if (Timer_Expired(sns_rfTransceive_RX_REPEATE_TIMER)) {
				rfRxChannel_state = sns_rfTransceive_STATE_IDLE;
			}
			break;

		default:
			break;
		}
#endif

#if IR_TX_ENABLE==1
		switch (rfTxChannel_state)
		{
		case sns_rfTransceive_STATE_IDLE:
		{
			/* transmission is started when a command is received on can */
			rfTxChannel_stopSend = FALSE;
			rfTxChannel_repeatCount = 0;
			rfTxChannel_proto.framecnt = 0;
			break;
		}

		case sns_rfTransceive_STATE_START_TRANSMIT:
		{
			/* Expand protocol. */
			if (expandProtocol(rfTxChannel_buf, &rfTxChannel_txlen, &rfTxChannel_proto) != IR_OK) {
				/* Failed to expand protocol -> enter idle state. */
				rfTxChannel_state = sns_rfTransceive_STATE_IDLE;
				break;
			}

			/* Start RF transmission. */
			IrTransceiver_Transmit(0, rfTxChannel_buf, 0, rfTxChannel_txlen, rfTxChannel_proto.modfreq);

			/* Enter transmitting state. */
			rfTxChannel_state = sns_rfTransceive_STATE_TRANSMITTING;
			break;
		}

		case sns_rfTransceive_STATE_TRANSMITTING:
		{
			if (rfTxChannel_sendComplete == TRUE)
			{
				cli();
				rfTxChannel_sendComplete = FALSE;
				sei();

				rfTxChannel_state = sns_rfTransceive_STATE_START_PAUSE;
			}
			break;
		}

		case sns_rfTransceive_STATE_START_PAUSE:
		{
			if (rfTxChannel_repeatCount < rfTxChannel_proto.repeats)
			{
				rfTxChannel_repeatCount++;
			}

			Timer_SetTimeout(sns_rfTransceive_TX_REPEATE_TIMER, rfTxChannel_proto.timeout, TimerTypeOneShot, 0);

			if (rfTxChannel_proto.framecnt != 255)
			{
				rfTxChannel_proto.framecnt++;
			}

			rfTxChannel_state = sns_rfTransceive_STATE_PAUSING;
			break;
		}

		case sns_rfTransceive_STATE_PAUSING:
		{
			if (Timer_Expired(sns_rfTransceive_TX_REPEATE_TIMER))
			{
				rfTxChannel_state = sns_rfTransceive_STATE_START_TRANSMIT;
			}

			/* Transmission is stopped when such command is recevied on can */
			if (rfTxChannel_stopSend == TRUE && rfTxChannel_repeatCount >= rfTxChannel_proto.repeats)
			{
				rfTxChannel_state = sns_rfTransceive_STATE_IDLE;
			}
			break;
		}
		default:
			break;
		}
#endif
}

void sns_rfTransceive_HandleMessage(StdCan_Msg_t *rxMsg)
{
	if (	StdCan_Ret_class(rxMsg->Header) == CAN_MODULE_CLASS_SNS && 
		StdCan_Ret_direction(rxMsg->Header) == DIRECTIONFLAG_TO_OWNER &&
		rxMsg->Header.ModuleType == CAN_MODULE_TYPE_SNS_RFTRANSCEIVE &&
		rxMsg->Header.ModuleId == sns_rfTransceive_ID)
	{
		switch (rxMsg->Header.Command)
		{
#if IR_TX_ENABLE==1
		case CAN_MODULE_CMD_RFTRANSCEIVE_DATABURST:
		{
			if (rfTxChannel_state == sns_rfTransceive_STATE_IDLE)
			{
				rfTxChannel_stopSend = TRUE;
			}
		}	/* Fall through, no break */
		case CAN_MODULE_CMD_RFTRANSCEIVE_DATASTART:
		{
			if (rfTxChannel_state == sns_rfTransceive_STATE_IDLE)
			{
				rfTxChannel_proto.protocol = rxMsg->Data[1];
				rfTxChannel_proto.data = rxMsg->Data[2];
				rfTxChannel_proto.data = rfTxChannel_proto.data<<8;
				rfTxChannel_proto.data |= rxMsg->Data[3];
				rfTxChannel_proto.data = rfTxChannel_proto.data<<8;
				rfTxChannel_proto.data |= rxMsg->Data[4];
				rfTxChannel_proto.data = rfTxChannel_proto.data<<8;
				rfTxChannel_proto.data |= rxMsg->Data[5];
				rfTxChannel_proto.data = rfTxChannel_proto.data<<8;
				rfTxChannel_proto.data |= rxMsg->Data[6];
				rfTxChannel_proto.data = rfTxChannel_proto.data<<8;
				rfTxChannel_proto.data |= rxMsg->Data[7];

				rfTxChannel_state = sns_rfTransceive_STATE_START_TRANSMIT;
			}
		}
		break;
		case CAN_MODULE_CMD_RFTRANSCEIVE_DATASTOP:
		{
			rfTxChannel_stopSend = TRUE;
		}
		break;
#endif
		}
	}
}

void sns_rfTransceive_List(uint8_t ModuleSequenceNumber)
{
	StdCan_Msg_t txMsg;

	StdCan_Set_class(txMsg.Header, CAN_MODULE_CLASS_SNS);
	StdCan_Set_direction(txMsg.Header, DIRECTIONFLAG_FROM_OWNER);
	txMsg.Header.ModuleType = CAN_MODULE_TYPE_SNS_RFTRANSCEIVE;
	txMsg.Header.ModuleId = sns_rfTransceive_ID;
	txMsg.Header.Command = CAN_MODULE_CMD_GLOBAL_LIST;
	txMsg.Length = 6;

	uint32_t HwId=BIOS_GetHwId();
	txMsg.Data[0] = HwId&0xff;
	txMsg.Data[1] = (HwId>>8)&0xff;
	txMsg.Data[2] = (HwId>>16)&0xff;
	txMsg.Data[3] = (HwId>>24)&0xff;

	txMsg.Data[4] = NUMBER_OF_MODULES;
	txMsg.Data[5] = ModuleSequenceNumber;

	while (StdCan_Put(&txMsg) != StdCan_Ret_OK);
}
