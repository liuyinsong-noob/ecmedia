package com.hisun.phone.core.voice.multimedia;

import com.hisun.phone.core.voice.CCPCallImpl;
import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.RecordProduct;
import com.hisun.phone.core.voice.model.RecordProduct.RecordProductType;
import com.hisun.phone.core.voice.model.RecourProductInfo;
import com.hisun.phone.core.voice.util.Log4Util;

public class RecordConsumer implements Runnable {

	private static RecordConsumer instance;
	
	public static RecordConsumer getInstance() {
		if(instance == null ) {
			instance = new RecordConsumer();
		}
		return instance;
	}
	
	private RecordConsumer(){
		new Thread(this).start();
	}
	
	public void run() {
		while(true)
		{
			try {
				Log4Util.d(Device.TAG ,  "RecordConsumer consume");
				RecordProduct product = RecordBlockingQueue.getInstance().consume();
				RecourProductInfo productInfo = null;
				if(product.productType == RecordProductType.ProductStart)
				{
					productInfo = null;
					product = RecordBlockingQueue.getInstance().consume();
					if(product.productType == RecordProductType.ProductInfo)
					{
						productInfo = product.info;
						CCPCallImpl.getInstance().getCallControlManager().UploadMediaChunked(productInfo.uniqueID, productInfo.fileName, productInfo.groupId, productInfo.userData);
					}
				}
				
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}
	
}
