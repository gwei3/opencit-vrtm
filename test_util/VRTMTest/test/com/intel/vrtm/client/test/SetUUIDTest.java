package com.intel.vrtm.client.test;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import javax.xml.bind.DatatypeConverter;

import com.intel.vrtm.client.Factory;
import com.intel.vrtm.client.RPCCall;
import com.intel.vrtm.client.RPClient;
import com.intel.vrtm.client.TCBuffer;

public class SetUUIDTest {
	static final String xmlRPCBlob=  "<?xml version='1.0'?>" 
			   + "<methodCall>"
			   + "<methodName>setUUID</methodName>"
			   + 	"<params>"
			   +		"<param>"
			   +			"<value><string>%s</string></value>"
			   +		"</param>"
			   +		"<param>"
			   +			"<value><string>%s</string></value>"
			   +		"</param>"
			   +		"<param>"
			   +			"<value><string>%s</string></value>"
			   +		"</param>"
			   +	"</params>"
			   + "</methodCall>";

	/**
	 * 
	 * @param args
	 *            args[0] : RPCore IP Address
	 *            args[1] : RPCore Port 
	 *            args[2] : RP ID
	 *            args[3] : VM UUID
	 *            args[4] : VDI UUID, You will get VMUUID from OpenStack Horizon
	 * 
	 * @throws IOException
	 */
	public static void main(String[] args) throws IOException {
		// Get TCBuffer Objct for required call
		TCBuffer tcBuffer = Factory.newTCBuffer(RPCCall.SET_UUID);

		// first replace the %s of xmlRPCBlob by VMUUID, rpcore accept all
		// method input arguments in base64 format
		String base64InputArgument = String.format(xmlRPCBlob,
				DatatypeConverter.printBase64Binary(args[2].getBytes()),
				DatatypeConverter.printBase64Binary(args[3].getBytes()),
				DatatypeConverter.printBase64Binary("".getBytes()));
		System.out.println(base64InputArgument);
		tcBuffer.setRPCPayload(base64InputArgument.getBytes());

		// create instance of RPClient, One instance of RPClient for App life
		// time is sufficient
		// to do processing
		RPClient rpClient = new RPClient(args[0], Integer.parseInt(args[1]));
		// send tcBuffer to rpcore
		tcBuffer = rpClient.send(tcBuffer);

		// process response
		// System.out.println("rpid = " + tcBuffer.getRpId());
		System.out.println("RPC Call Index =" + tcBuffer.getRPCCallIndex());
		System.out
				.println("RPC Payload Size = " + tcBuffer.getRPCPayloadSize());
		System.out.println("RPC Call Status = " + tcBuffer.getRPCCallStatus());
		// System.out.println("RPC Original RP ID = " +
		// tcBuffer.getOriginalRpId());
		ByteBuffer b = ByteBuffer.allocate(4);
		b.order(ByteOrder.LITTLE_ENDIAN);
		b.clear();
		b.put(tcBuffer.getRPCPayload().getBytes());
		b.rewind();
		System.out.println("RPC Payload = " +  b.getInt());
		// close RPClient at the end of application
		rpClient.close();

	}

}
