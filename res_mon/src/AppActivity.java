/**
 * A2 Embedded Systems - Resource Monitor
 * Mihail Dunaev
 * 19 Jan, 2014
 * AppActivity.java - displays info about apps
 */

package temasi.resource;

import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.content.Context;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.SystemClock;
import android.util.SparseArray;
import android.view.Menu;
import android.widget.ExpandableListView;

public class AppActivity extends Activity {

	public static final int BYTES_IN_MB = 1048576;
	public static final int BYTES_IN_KB = 1024;
	
	public static final int STATS_TOTAL = 0; 
	public static final int GPS_TYPE = -10000;
	public static final int STATS_SINCE_CHARGED = 0;
	
	public static final String POWER_CPU_IDLE = "cpu.idle";
	public static final String POWER_CPU_AWAKE = "cpu.awake";
	public static final String POWER_CPU_ACTIVE = "cpu.active";
	public static final String POWER_GPS_ON = "gps.on";
	
    public static final int NETWORK_WIFI_RX_BYTES = 2;
    public static final int NETWORK_WIFI_TX_BYTES = 3;
	
	private static final String BATTERY_STATS_IMPL_CLASS = "com.android.internal.os.BatteryStatsImpl";
	private static final String BATTERY_STATS_CLASS = "android.os.BatteryStats";
	private static final String UID_CLASS = BATTERY_STATS_CLASS + "$Uid";
	private static final String PROC_CLASS = UID_CLASS + "$Proc";
	private static final String M_BATTERY_INFO_CLASS = "com.android.internal.app.IBatteryStats";
	private static final String POWER_PROFILE_CLASS = "com.android.internal.os.PowerProfile";
	
	long battery_realtime;
	Object battery_info, stats, power_profile;
	
	List<String> app_names;
	HashMap<String, List<String>> app_infos;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		
		// standard
		super.onCreate(savedInstanceState);
		setContentView(R.layout.apps);
		
		// apps
		set_apps_info ();
	}
	
	public void set_apps_info () {
		
		app_names = new ArrayList<String>();
		app_infos = new HashMap<String, List<String>>();
		ExpandableListView app_elv = (ExpandableListView)findViewById(R.id.expLV1);
		
		int count = 0;
		int which = STATS_TOTAL;
		Integer uid;
		
		// read stats for every process
		init_stats();

		// get only procs;
		ActivityManager am = (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
		List<RunningAppProcessInfo> running_processes = am.getRunningAppProcesses();
		Iterator<RunningAppProcessInfo> it = running_processes.iterator();
		RunningAppProcessInfo tmp_prc;
		List<String> only_procs = new ArrayList<String>();
		
		while (it.hasNext()){
			tmp_prc = it.next();
			only_procs.add(tmp_prc.processName);
		}
		
		// total CPU time in ms
		long cpu_time = 0;
		
		try {
			SparseArray uid_stats = (SparseArray)Class.forName(BATTERY_STATS_IMPL_CLASS).getMethod("getUidStats", (Class[])null).invoke(stats, (Object[])null);
			final int N = uid_stats.size();
			for (int i = 0; i < N; i++){
				Object uid_stat = uid_stats.valueAt(i);
				Map<String, Object> process_stats = (Map)Class.forName(UID_CLASS).getMethod("getProcessStats", (Class[])null).invoke(uid_stat, (Object[])null);			
				if(process_stats.size() > 0){					
					for (Map.Entry<String, Object> entry: process_stats.entrySet()){
						
						// don't show info about kernel or wakelocks
						if (only_procs.contains(entry.getKey()) == false)
							continue;
						
						// save proc name + uid
						uid = (Integer)uid_stat.getClass().getMethod("getUid", null).invoke(uid_stat, (Object[]) null);
						app_names.add(entry.getKey() + " - " + String.valueOf(uid));

						// get aditional info
						List<String> app_info = new ArrayList<String>();
						
						// Proc ps;
						Object ps = entry.getValue();
						
						// cpu_time = user_time + system_time;
						long user_time = (Long)Class.forName(PROC_CLASS).getMethod("getUserTime", java.lang.Integer.TYPE).invoke(ps, which);
						long system_time = (Long)Class.forName(PROC_CLASS).getMethod("getSystemTime", java.lang.Integer.TYPE).invoke(ps, which);
						cpu_time = (user_time + system_time) * 10;
						
						app_info.add("CPU time ".concat(String.valueOf(cpu_time)).concat(" ms"));
						
						// CPU energy : (power (mW) x time_on_cpu (ms)) / 1000 -> mJ
						double cpu_power = (Double) Class.forName(POWER_PROFILE_CLASS).getMethod("getAveragePower", String.class)
											.invoke(power_profile, POWER_CPU_ACTIVE);  // mW
						
						double energy_cpu = (cpu_power * (double)cpu_time) / 1000;
						app_info.add("Energy for CPU ".concat(String.valueOf(energy_cpu)).concat(" mJ"));
						
						// get GPS time
						Map<Integer, Object> sensor_stats = (Map)Class.forName(UID_CLASS).getMethod("getSensorStats", (Class[])null).invoke(uid_stat);
						if (sensor_stats.size() == 0){
							app_info.add("GPS time 0 ms");
							app_info.add("Energy for GPS 0.0 mJ");
						}
						
						if(sensor_stats.size() > 0){
							long gps_time = 0;
							for (Map.Entry<Integer, Object> entry_sen: sensor_stats.entrySet()){
								Object sensor = entry_sen.getValue();
								int sensor_type = (Integer)sensor.getClass().getMethod("getHandle", (Class[])null).invoke(sensor, (Object[]) null);
								if (sensor_type == GPS_TYPE){
									Object timer_gps = sensor.getClass().getMethod("getSensorTime", (Class[])null).invoke(sensor, (Object[]) null);
									
									// battery_realtime
									battery_realtime = (Long)Class.forName(BATTERY_STATS_IMPL_CLASS)
											.getMethod("computeBatteryRealtime", java.lang.Long.TYPE, java.lang.Integer.TYPE)
											.invoke(stats, SystemClock.elapsedRealtime() * 1000, STATS_TOTAL);
									
									gps_time = (Long)timer_gps.getClass().getMethod("getTotalTimeLocked", java.lang.Long.TYPE, java.lang.Integer.TYPE)
												 .invoke(timer_gps, battery_realtime, which);
									gps_time = gps_time / 1000;
								}
							}
							
							app_info.add("GPS time ".concat(String.valueOf(gps_time)).concat(" ms"));
							
							// GPS energy
							double gps_power = (Double)Class.forName(POWER_PROFILE_CLASS).getMethod("getAveragePower", String.class)
												.invoke(power_profile, POWER_GPS_ON);  // mW
							
							double energy_gps = (gps_power * (double)gps_time) / 1000;
							app_info.add("Energy for GPS ".concat(String.valueOf(energy_gps)).concat(" mJ"));
						}
						
						app_infos.put(app_names.get(count), app_info);
						count++;
					}
				}
			}
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
		
        ExpandableListAdapter listAdapter = new ExpandableListAdapter(this, app_names, app_infos);
        app_elv.setAdapter(listAdapter);
	}
	
	public void init_stats () {
		
		byte[] data;
		try {
			battery_info = Class.forName("com.android.internal.app.IBatteryStats$Stub").getDeclaredMethod("asInterface", 
					IBinder.class).invoke(null, Class.forName("android.os.ServiceManager")
							.getMethod("getService", String.class).invoke(null, "batteryinfo"));
			
			// power_profile = new PowerProfile(this);
			power_profile = Class.forName(POWER_PROFILE_CLASS).getConstructor(Context.class).newInstance(this);
			
			// data = battery_info.getStatistics();
			data = (byte[])Class.forName(M_BATTERY_INFO_CLASS).getMethod("getStatistics", (Class[])null).invoke(battery_info, (Object[])null);
			
			// parcel = new parcel(data);
			Parcel parcel = Parcel.obtain();
			parcel.unmarshall(data, 0, data.length);
			parcel.setDataPosition(0);
			
			// stats = BatteryStatsImpl.createFromParcel (parcel);
			stats = Class.forName(BATTERY_STATS_IMPL_CLASS).getField("CREATOR").getType().getMethod("createFromParcel", Parcel.class)
			.invoke(Class.forName(BATTERY_STATS_IMPL_CLASS).getField("CREATOR").get(null), parcel);
			
		} catch (IllegalAccessException e1) {
			e1.printStackTrace();
		} catch (IllegalArgumentException e1) {
			e1.printStackTrace();
		} catch (InvocationTargetException e1) {
			e1.printStackTrace();
		} catch (NoSuchMethodException e1) {
			e1.printStackTrace();
		} catch (ClassNotFoundException e1) {
			e1.printStackTrace();
		} catch (NoSuchFieldException e) {
			e.printStackTrace();
		} catch (InstantiationException e) {
			e.printStackTrace();
		}
	}
	
	// Not used atm
	public void update_apps_info (){

		init_stats();
		int which = STATS_TOTAL;

		try {
			// Array<Uid> uid_stats = stats.getUidStats();
			SparseArray uid_stats = (SparseArray)Class.forName(BATTERY_STATS_IMPL_CLASS).getMethod("getUidStats", (Class[])null).invoke(stats, (Object[])null);
			final int N = uid_stats.size();
			
			for (int i = 0; i < N; i++){
				// Uid uid_stat;
				Object uid_stat = uid_stats.valueAt(i);
				// Map<String, Proc> process_stats;
				Map<String, Object> process_stats = (Map)Class.forName(UID_CLASS).getMethod("getProcessStats", (Class[])null).invoke(uid_stat, (Object[])null);
				
				if(process_stats.size() > 0){
					for (Map.Entry<String, Object> entry: process_stats.entrySet()){
						// same old same old
						Integer uid = (Integer)uid_stat.getClass().getMethod("getUid", null).invoke(uid_stat, (Object[]) null);
						String app_id = entry.getKey().concat(" - ").concat(String.valueOf(uid));
						
						if (app_names.contains(app_id) == false)
							continue;

						// get the info list
						List<String> app_info = app_infos.get(app_id);
						
						// Proc ps;
						Object ps = entry.getValue();
						
						// cpu_time = user_time + system_time;
						// user_time = ps.getUserTime(); system_time = ps.getSystemTime();
						long user_time = (Long)Class.forName(PROC_CLASS).getMethod("getUserTime", java.lang.Integer.TYPE).invoke(ps, which);
						long system_time = (Long)Class.forName(PROC_CLASS).getMethod("getSystemTime", java.lang.Integer.TYPE).invoke(ps, which);
						long cpu_time = (user_time + system_time) * 10;
						
						app_info.set(0, "CPU time ".concat(String.valueOf(cpu_time)).concat(" ms"));
						
						// CPU energy : (power (mW) x time_on_cpu (ms)) / 1000 -> mJ
						double cpu_power = (Double)Class.forName(POWER_PROFILE_CLASS).getMethod("getAveragePower", String.class)
											.invoke(power_profile, POWER_CPU_ACTIVE);  // mW
						
						double energy_cpu = (cpu_power * (double)cpu_time) / 1000;
						app_info.set(1, "Energy for CPU ".concat(String.valueOf(energy_cpu)).concat(" mJ"));
						
						// get GPS time
						Map<Integer, Object> sensor_stats = (Map)Class.forName(UID_CLASS).getMethod("getSensorStats", (Class[])null).invoke(uid_stat);
						if (sensor_stats.size() == 0){
							app_info.set(2, "GPS time 0 ms");
							app_info.set(3, "Energy for GPS 0.0 mJ");
						}
						
						if(sensor_stats.size() > 0){
							
							long gps_time = 0;
							for (Map.Entry<Integer, Object> entry_sen: sensor_stats.entrySet()){
								Object sensor = entry_sen.getValue();
								int sensor_type = (Integer)sensor.getClass().getMethod("getHandle", (Class[])null).invoke(sensor, (Object[]) null);
															
								if (sensor_type == GPS_TYPE){
									
									Object timer_gps = sensor.getClass().getMethod("getSensorTime", (Class[])null).invoke(sensor, (Object[]) null);
									
									// battery_realtime
									battery_realtime = (Long)Class.forName(BATTERY_STATS_IMPL_CLASS)
											.getMethod("computeBatteryRealtime", java.lang.Long.TYPE, java.lang.Integer.TYPE)
											.invoke(stats, SystemClock.elapsedRealtime() * 1000, STATS_TOTAL);
									
									gps_time = (Long)timer_gps.getClass().getMethod("getTotalTimeLocked", java.lang.Long.TYPE, java.lang.Integer.TYPE)
												.invoke(timer_gps, battery_realtime, which);
									
									gps_time = gps_time / 1000;
								}
							}
							
							app_info.set(2, "GPS time ".concat(String.valueOf(gps_time)).concat(" ms"));
							
							// GPS energy						
							double gps_power = (Double)Class.forName(POWER_PROFILE_CLASS).getMethod("getAveragePower", String.class)
												.invoke(power_profile, POWER_GPS_ON);  // mW
							
							double energy_gps = (gps_power * (double)gps_time) / 1000;
							app_info.add(3, "Energy for GPS ".concat(String.valueOf(energy_gps)).concat(" mJ"));
						}
					}
				}
			}
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
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
}
