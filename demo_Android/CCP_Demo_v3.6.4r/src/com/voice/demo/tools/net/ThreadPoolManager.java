/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *   http://www.cloopen.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
package com.voice.demo.tools.net;

import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

/**
 * Processing the network operation request
 * @version Time: 2013-7-26
 */
public class ThreadPoolManager {
    private static ThreadPoolManager instance = null;
    private List<ITask> taskQueue = Collections.synchronizedList(new LinkedList<ITask>());// The task queue
    private WorkThread[] workQueue ;    // The worker thread (the real task execution thread)
    private static int worker_num = 5;    // The number of worker threads (5 is the default number of worker threads)
    
    private ThreadPoolManager(){
        this(5);
    }
    private ThreadPoolManager(int num){
        worker_num = num;
        workQueue = new WorkThread[worker_num];
        for(int i=0;i<worker_num;i++){
            workQueue[i] = new WorkThread(i);
        }
    }
    
    public static synchronized ThreadPoolManager getInstance(){
        if(instance==null)
            instance = new ThreadPoolManager();
        return instance;
    }
    
    public void addTask(ITask task){
        // On the task queue to be locked
        synchronized (taskQueue) {
            if(task!=null){
                taskQueue.add(task);
                taskQueue.notifyAll();
            }
                
        }
    }
    
    public void BatchAddTask(ITask[] tasks){
        // On the task queue modification operations be locked
        synchronized (taskQueue) {
            for(ITask e:tasks){
                if(e!=null){
                    taskQueue.add(e);
                    taskQueue.notifyAll();
                }
            }        
        }
    }
    
    public void destory(){
        for(int i = 0;i<worker_num;i++){
            workQueue[i].stopThread();
            workQueue[i] = null;
        }
        // On the task queue to be locked
        synchronized (taskQueue) {
            taskQueue.clear();
        }
        
    }
    
    private class WorkThread extends Thread{
        //private int taksId ;
        private boolean isRuning = true;
        //private boolean isWaiting = false;
        
        
         
        public WorkThread(int taskId){
           // this.taksId= taskId;
            this.start();
        }
        
        /*public boolean isWaiting(){
            return isWaiting;
        }*/
        // If the task is, not immediately terminate the thread, 
        // then wait for the task to detect isRuning false, quit run () method
        public void stopThread(){
            isRuning = false;
        }
        
        @Override
        public void run() {
            while(isRuning){
                ITask iTask = null;
                // On the task queue to be locked
                synchronized (taskQueue) {
                    // The task queue is empty, waiting for the new task to join
                    while(isRuning&&taskQueue.isEmpty()){
                        try {
                            taskQueue.wait(20);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                    if(isRuning)
                    	iTask = taskQueue.remove(0);
                }
                // When waiting for the new task joins, thread termination 
                // (call stopThread function by temp = null)
                if(iTask!=null){
                    //isWaiting = false;
                	if(iTask.getParamsCount() > 0) {
                		
                	}
                	try {
                		Thread.sleep(200);
                	} catch (InterruptedException e) {
                		e.printStackTrace();
                	}
                    if(mLinstner != null ) {
                    	mLinstner.doTaskBackGround(iTask);
                    }
                    //isWaiting = true;
                }    
            }
        }
    }
    
    public OnTaskDoingLinstener mLinstner;
    public interface OnTaskDoingLinstener{
		
		void doTaskBackGround(ITask iTask);
	}
    
    
	public void setOnTaskDoingLinstener(OnTaskDoingLinstener linstner) {
    	this.mLinstner = linstner;
    }
}