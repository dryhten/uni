/**
 * A2 Embedded Systems - Resource Monitor
 * Mihail Dunaev
 * 19 Jan, 2014
 * MainActivity.java - displays basic info about phone
 */

package temasi.resource;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Timer;
import java.util.TimerTask;

import android.net.TrafficStats;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.BatteryManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.StatFs;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.view.*;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.os.Build;

public class MainActivity extends Activity {

	public static final int BYTES_IN_MB = 1048576;
	public static final int BYTES_IN_KB = 1024;
	
	public static final int STATS_TOTAL = 0; 
	public static final int GPS_TYPE = -10000;
	
	public static final String POWER_WIFI_ON = "wifi.on";
	public static final String POWER_WIFI_ACTIVE = "wifi.active";
	public static final String POWER_WIFI_SCAN = "wifi.scan";
	
	public static final int RAM_UPDATE_INTERVAL = 1000;  // ms
	public static final int BATTERY_UPDATE_INTERVAL = 600000;  // ms
	public static final int WI_FI_UPDATE_INTERVAL = 5000;// 5000;  // ms
	
    public static final int NETWORK_WIFI_RX_BYTES = 2;
    public static final int NETWORK_WIFI_TX_BYTES = 3;
    
    private static final String POWER_PROFILE_CLASS = "com.android.internal.os.PowerProfile";
	
	TextView model_dev, ver_android, ram_total, ram_free, disk_total, disk_free, cpu, battery_level,
			 wifi_rx, wifi_tx, wifi_energy;
	
	Object power_profile;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		
		// standard
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		// set basic infos
		set_basic_info();
		
		// set app button
		set_app_button();
	}
	
	public void set_basic_info (){
		
		// what to set
		model_dev = (TextView)findViewById(R.id.model_dev_id);
		ver_android = (TextView)findViewById(R.id.ver_android_id);
		ram_total = (TextView)findViewById(R.id.ram_total_id);
		ram_free = (TextView)findViewById(R.id.ram_free_id);
		disk_total = (TextView)findViewById(R.id.disk_total_id);
		disk_free = (TextView)findViewById(R.id.disk_free_id);
		cpu = (TextView)findViewById(R.id.cpu_id);
		battery_level = (TextView)findViewById(R.id.battery_level_id);
		wifi_tx = (TextView)findViewById(R.id.wifi_tx_id);
		wifi_rx = (TextView)findViewById(R.id.wifi_rx_id);
		wifi_energy = (TextView)findViewById(R.id.wifi_energy_id);
		
		// static info
		set_basic_model();
		set_basic_ver();
		set_basic_total_ram();
		set_basic_disk();
		set_basic_cpu();
		
		// dynamic info
		set_basic_free_ram();
		set_basic_battery();
		set_basic_wifi_tx();
		set_basic_wifi_rx();
		set_basic_wifi_energy();
	}
	
	public void set_basic_model (){
		model_dev.setText(Build.MANUFACTURER + " " + Build.MODEL);
	}
	
	public void set_basic_ver (){
		ver_android.setText(Build.VERSION.RELEASE);
	}
	
	public void set_basic_total_ram (){
		set_total_ram();
	}
	
	@SuppressLint("NewApi")
	@SuppressWarnings("deprecation")
	public void set_basic_disk (){
	
		// physical disk space - using fstat
		StatFs aux_sf;
		long block_size, block_count, avail_blocks, aux_disk_total, aux_disk_free;
		
		// external storage
		aux_sf = new StatFs(Environment.getExternalStorageDirectory().getAbsolutePath());
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2){
			block_size = aux_sf.getBlockSizeLong();
			block_count = aux_sf.getBlockCountLong();
			avail_blocks = aux_sf.getAvailableBlocksLong();
		} else {
			block_size = aux_sf.getBlockSize();
			block_count = aux_sf.getBlockCount();
			avail_blocks = aux_sf.getAvailableBlocks();
		}
		
		aux_disk_total = (block_size * block_count) / BYTES_IN_MB;
		aux_disk_free = (block_size * avail_blocks) / BYTES_IN_MB;
		
		// root storage
		aux_sf = new StatFs(Environment.getRootDirectory().getAbsolutePath());
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2){
			block_size = aux_sf.getBlockSizeLong();
			block_count = aux_sf.getBlockCountLong();
			avail_blocks = aux_sf.getAvailableBlocksLong();
		} else {
			block_size = aux_sf.getBlockSize();
			block_count = aux_sf.getBlockCount();
			avail_blocks = aux_sf.getAvailableBlocks();
		}
		
		aux_disk_total = aux_disk_total + ((block_size * block_count) / BYTES_IN_MB);
		aux_disk_free = aux_disk_free + ((block_size * avail_blocks) / BYTES_IN_MB);
		
		
		disk_total.setText(String.valueOf(aux_disk_total).concat(" MB"));
		disk_free.setText(String.valueOf(aux_disk_free).concat(" MB"));
		
		/* physical disk space - v2; requires API > 18
		aux_disk_total = aux_sf.getTotalBytes() / BYTES_IN_MB;
		aux_disk_free = aux_sf.getAvailableBytes() / BYTES_IN_MB;
		*/
		
		/* logical
		Stack<File> dir_stack = new Stack<File>();
		dir_stack.push(new File("/"));
		
		File it_file;
		long used_space = 0;
		
		while (dir_stack.isEmpty() == false){
			
			it_file = dir_stack.pop();
			File[] file_list = it_file.listFiles();
			for (int i = 0; i < file_list.length; i++){
				if (file_list[i].isDirectory())
					dir_stack.push(file_list[i]);
				else
					used_space = used_space + file_list[i].length();
			}
		} */
		
		/* using adb "du" */
	}
	
	public void set_basic_cpu (){
		cpu.setText(System.getProperty("os.arch"));
	}
	
	public void set_basic_free_ram(){
		Timer free_ram_t = new Timer();
		TimerTask free_ram_task = new TimerTask(){
			@Override
			public void run() {
				runOnUiThread(new Runnable(){
					@Override
					public void run() {
						set_free_ram();
					}
				});
			}
		};
		free_ram_t.schedule(free_ram_task, 0, RAM_UPDATE_INTERVAL);
	}
	
	public void set_basic_battery() {
		Timer battery_level_t = new Timer();
		TimerTask battery_level_task = new TimerTask(){
			@Override
			public void run() {
				runOnUiThread(new Runnable(){
					@Override
					public void run() {
						set_battery_level();
					}
				});
			}
		};
		battery_level_t.schedule(battery_level_task, 0, BATTERY_UPDATE_INTERVAL);
	}
	
	public void set_basic_wifi_tx(){
		Timer wifi_tx_t = new Timer();
		TimerTask wifi_tx_task = new TimerTask(){
			@Override
			public void run() {
				runOnUiThread(new Runnable(){
					@Override
					public void run() {
						set_wifi_tx();
					}
				});
			}
		};
		wifi_tx_t.schedule(wifi_tx_task, 0, WI_FI_UPDATE_INTERVAL);
	}
	
	public void set_basic_wifi_rx(){
		Timer wifi_rx_t = new Timer();
		TimerTask wifi_rx_task = new TimerTask(){
			@Override
			public void run(){
				runOnUiThread(new Runnable(){
					@Override
					public void run(){
						set_wifi_rx();
					}
				});
			}
		};
		wifi_rx_t.schedule(wifi_rx_task, 0, WI_FI_UPDATE_INTERVAL);
	}
	
	public void set_basic_wifi_energy(){
		
		try {
			power_profile = Class.forName(POWER_PROFILE_CLASS).getConstructor(Context.class).newInstance(this);
		} catch (InstantiationException e) {
			e.printStackTrace();
		} catch (IllegalAccessException e) {
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			e.printStackTrace();
		} catch (InvocationTargetException e) {
			e.printStackTrace();
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
		}
		
		Timer wifi_energy_t = new Timer();
		TimerTask wifi_energy_task = new TimerTask(){
			@Override
			public void run () {
				runOnUiThread(new Runnable(){
					@Override
					public void run(){
						set_wifi_energy();
					}
				});
			}
		};
		wifi_energy_t.schedule(wifi_energy_task, 0, WI_FI_UPDATE_INTERVAL);
		
	}
	
	public void set_wifi_tx (){
		long aux_tx = TrafficStats.getTotalTxBytes() - TrafficStats.getMobileTxBytes();
		wifi_tx.setText(String.valueOf(aux_tx));
	}

	public void set_wifi_rx (){
		long aux_rx = TrafficStats.getTotalRxBytes() - TrafficStats.getMobileRxBytes();
		wifi_rx.setText(String.valueOf(aux_rx));
	}
	
	public void set_wifi_energy (){
		
		WifiManager wifi_man = (WifiManager)getSystemService(Context.WIFI_SERVICE);
		WifiInfo wifi_info = wifi_man.getConnectionInfo();
		Integer link_speed = wifi_info.getLinkSpeed();
		
		link_speed = link_speed * (1000000 / 8);
		long total_traffic_snd = TrafficStats.getTotalTxBytes() - TrafficStats.getMobileTxBytes();
		long total_traffic_rcv = TrafficStats.getTotalRxBytes() - TrafficStats.getMobileRxBytes();
		long total_traffic = Math.max(total_traffic_snd, total_traffic_rcv);
		long time_wifi_used = (total_traffic * 1000) / (long)link_speed;  // ms
		
		double energy_wifi = 0;
		
		try {
			
			// get power in mW
			double wifi_power = (Double)Class.forName(POWER_PROFILE_CLASS).getMethod("getAveragePower", String.class)
					.invoke(power_profile, POWER_WIFI_ACTIVE);
			energy_wifi = wifi_power * (double)time_wifi_used;
			energy_wifi = energy_wifi / 1000;
			wifi_energy.setText(String.valueOf(energy_wifi).concat(" mJ"));
		} catch (IllegalAccessException e) {
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			e.printStackTrace();
		} catch (InvocationTargetException e) {
			e.printStackTrace();
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
		}
	}
	
	public void set_app_button (){
		
		Button app_button = (Button)findViewById(R.id.app_button);
		app_button.setOnClickListener(new OnClickListener (){

			@Override
			public void onClick(View arg0) {
				Intent my_intent = new Intent(arg0.getContext(), AppActivity.class);
				arg0.getContext().startActivity(my_intent);	}
		});
	}
	
	public void set_total_ram (){
		
		try {
			Class<?> c = Class.forName("com.android.internal.util.MemInfoReader");
			Object t = c.newInstance();
			Method[] allMethods = c.getDeclaredMethods();

			for (Method m : allMethods) {
				String mname = m.getName();
				if (mname.compareTo("readMemInfo") == 0){
					try {
						m.setAccessible(true);
						m.invoke(t, (Object[]) null);
					} catch (IllegalArgumentException e) {
						e.printStackTrace();
					} catch (InvocationTargetException e) {
						e.printStackTrace();
					}
				}
			}
			
			for (Method m : allMethods) {
				String mname = m.getName();
				if (mname.compareTo("getTotalSize") == 0){
					Object o;
					try {
						m.setAccessible(true);
						o = m.invoke(t, (Object[]) null);
						Long aux = (Long)o;
						aux = aux / BYTES_IN_KB;
						ram_total.setText(String.valueOf(aux) + " kB");
					} catch (IllegalArgumentException e) {
						e.printStackTrace();
					} catch (InvocationTargetException e) {
						e.printStackTrace();
					}
				}
			}
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
		} catch (InstantiationException x) {
			x.printStackTrace();
		} catch (IllegalAccessException x) {
			x.printStackTrace();
		}
	}
	
	public void set_free_ram (){
		
		try {
			Class<?> c = Class.forName("com.android.internal.util.MemInfoReader");
			Object t = c.newInstance();
			Method[] allMethods = c.getDeclaredMethods();

			for (Method m : allMethods) {
				String mname = m.getName();
				if (mname.compareTo("readMemInfo") == 0){
					try {
						m.setAccessible(true);
						m.invoke(t, (Object[]) null);
					} catch (IllegalArgumentException e) {
						e.printStackTrace();
					} catch (InvocationTargetException e) {
						e.printStackTrace();
					}
				}
			}
			
			for (Method m : allMethods) {
				String mname = m.getName();
				if (mname.compareTo("getFreeSize") == 0){
					Object o;
					try {
						m.setAccessible(true);
						o = m.invoke(t, (Object[]) null);
						Long aux = (Long)o;
						aux = aux / BYTES_IN_KB;
						ram_free.setText(String.valueOf(aux) + " kB");
					} catch (IllegalArgumentException e) {
						e.printStackTrace();
					} catch (InvocationTargetException e) {
						e.printStackTrace();
					}
				}
			}
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
		} catch (InstantiationException x) {
			x.printStackTrace();
		} catch (IllegalAccessException x) {
			x.printStackTrace();
		}
	}

	public void set_battery_level (){
		
		IntentFilter ifilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
		Intent battery_status = getApplicationContext().registerReceiver(null, ifilter);
		
		int level_rel = battery_status.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
		int level_max = battery_status.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
		float level_abs = (level_rel * 100) / (float)level_max;
		
		battery_level.setText(String.valueOf(level_abs) + " %");
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
}
