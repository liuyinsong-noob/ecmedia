package com.hisun.phone.core.voice.multimedia;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import com.hisun.phone.core.voice.Device;
import com.hisun.phone.core.voice.model.RecordProduct;
import com.hisun.phone.core.voice.model.RecordProduct.RecordProductType;
import com.hisun.phone.core.voice.util.Log4Util;


public class RecordBlockingQueue {
	
	private static RecordBlockingQueue instance;
	
	public static RecordBlockingQueue getInstance() {
		if(instance == null ) {
			instance = new RecordBlockingQueue();

		}
		
		return instance;
	}
	// 阻塞队列
	BlockingQueue<RecordProduct> queue = new LinkedBlockingQueue<RecordProduct>();
	
	// 生成者，放入数据内容到队列
	public void produce(RecordProduct data) throws InterruptedException {
		// put方法放入一个数据
		queue.put(data);
		if(data.productType == RecordProductType.ProductData)
			Log4Util.d(Device.TAG , "put type:"+data.productType + ";data length:"+data.data.length);
		else
			Log4Util.d(Device.TAG , "put type:"+data.productType);
	}
	
	// 消费者，从队列中获取数据
	public  RecordProduct consume() throws InterruptedException {
		// take方法取出一个数据
		RecordProduct take = queue.take();
		
		if(take.productType == RecordProductType.ProductData) {
			Log4Util.d(Device.TAG , "take type="+take.productType + ";data length:"+take.data.length);
		} else {
			Log4Util.d(Device.TAG , "take type:"+take.productType);
		}
		return take;
	}
	
	public void clear()
	{
		queue.clear();
	}
}
