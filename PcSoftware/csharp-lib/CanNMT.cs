using System.Threading;
using System;

public class CanNMT {
	private const byte CAN_NMT				= 0x00;
	
	private const byte CAN_NMT_RESET 		= 0x04;
	private const byte CAN_NMT_BIOS_START 	= 0x08;
	private const byte CAN_NMT_PGM_START 	= 0x0C;
	private const byte CAN_NMT_PGM_DATA 	= 0x10;
	private const byte CAN_NMT_PGM_END 		= 0x14;
	private const byte CAN_NMT_PGM_COPY		= 0x18;
	private const byte CAN_NMT_PGM_ACK 		= 0x1C;
	private const byte CAN_NMT_PGM_NACK 	= 0x20;
	private const byte CAN_NMT_START_APP 	= 0x24;
	private const byte CAN_NMT_APP_START 	= 0x28;
	private const byte CAN_NMT_HEARTBEAT 	= 0x2C;

	CanPacket cp;
	private bool running = false;
	
	public CanNMT() {
		cp = new CanPacket();
		cp.setExt(1);
	}
	
	public void setSender(byte id) {
		cp.setId( ( ( cp.getId()&0x00FF00FF )|((uint)id<<8) ) ); 
	}
	public void setReceiver(byte id) {
		cp.setId( ( ( cp.getId()&0x00FFFF00 )|((uint)id) ) ); 
	}
	
	public CanPacket getResetPacket() {
		cp.setId( ( cp.getId()&0x0000FFFF )|( CAN_NMT_RESET<<16 ) ); 
		cp.setDataLength(0);
		return cp;
	}

	public CanPacket getStartPacket() {
		cp.setId( ( cp.getId()&0x0000FFFF )|( CAN_NMT_START_APP<<16 ) ); 
		cp.setDataLength(0);
		return cp;
	}
	
	public bool isBiosStart(CanPacket cp, byte sender) {
		bool returnval = false;
		uint id = cp.getId();
		if ((id&0x00FF0000) == (CAN_NMT_BIOS_START<<16) && (id&0x0000FF00) == (sender<<8)) {
			returnval = true;
			byte[] data = cp.getData();
			string appinfo = "Application installed";
			if (data[2] == 0) {
				appinfo = "No application installed";
			}
			Console.WriteLine("Bios started on node 0x" + String.Format("{0:x2}", sender) + 
								", bios version 0x" + String.Format("{0:x2}", data[1]) + String.Format("{0:x2}", data[0]) +
								", " + appinfo
								);
		}
		return returnval;
	}

	public bool doStart(DaemonConnection dc, byte receiver) {
		return doStart(dc, receiver, 0);
	}
	
	public bool doStart(DaemonConnection dc, byte receiver, byte sender) {
		bool success = true;
		
		setReceiver(receiver);
		dc.sendCanPacket(getStartPacket());
		return success;
	}

	public bool doReset(DaemonConnection dc, byte receiver) {
		return doReset(dc, receiver, 0);
	}
	
	public bool doReset(DaemonConnection dc, byte receiver, byte sender) {
		bool success = false;
		
		setReceiver(receiver);
		dc.sendCanPacket(getResetPacket());
		CanPacket getcp;
		running = true;
		dc.flushData();
		for (int i = 0; i < 150; i++) {
			Thread.Sleep(50);
			if (dc.getData(out getcp)) {
				if (isBiosStart(getcp, receiver)) {
					success = true;
					break;
				}
			}
			
			if (!running) {
				break;
			}
		}
		if (!success) {
			Console.WriteLine("Timed out while waiting for node to reset");
		}
		return success;
	}
	
	public void stop() {
		running = false;
	}
	
}
