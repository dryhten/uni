/**
 * A2 Embedded Systems - Resource Monitor
 * Mihail Dunaev
 * 19 Jan, 2014
 * ExpandableListAdapter.java - expandable list with apps
 */

package temasi.resource;

import java.util.HashMap;
import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.TextView;

public class ExpandableListAdapter extends BaseExpandableListAdapter {

    private Context context;
    private List<String> first_list;
    private HashMap<String, List<String>> second_list;
 
    public ExpandableListAdapter(Context context, List<String> first_list,
            HashMap<String, List<String>> second_list) {
    	
        this.context = context;
        this.first_list = first_list;
        this.second_list = second_list;
    }
	
	
	@Override
	public Object getChild(int arg0, int arg1) {
        return this.second_list.get(this.first_list.get(arg0)).get(arg1);
	}

	@Override
	public long getChildId(int arg0, int arg1) {
		return arg1;
	}

	@Override
	public View getChildView(int arg0, int arg1, boolean arg2, View arg3,
			ViewGroup arg4) {
		
        final String childText = (String)getChild(arg0, arg1);
        
        if (arg3 == null) {
            LayoutInflater inflater = (LayoutInflater)this.context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            arg3 = inflater.inflate(R.layout.list_item, null);
        }
 
        TextView list_child = (TextView)arg3.findViewById(R.id.process_info);
        list_child.setText(childText);
        return arg3;
	}

	
	@Override
	public int getChildrenCount (int arg0) {
		return this.second_list.get(this.first_list.get(arg0)).size();
	}
	
	@Override
	public Object getGroup(int arg0) {
		return this.first_list.get(arg0);
	}
	 
	@Override
	public int getGroupCount() {
		return this.first_list.size();
	}
	
	@Override
	public long getGroupId(int arg0) {
		return arg0;
	}
	
	@Override
	public View getGroupView(int arg0, boolean arg1, View arg2, ViewGroup arg3) {
		
		String title = (String)getGroup (arg0);
		
		if (arg2 == null){
			LayoutInflater inflater = (LayoutInflater)this.context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			arg2 = inflater.inflate(R.layout.list_group, null);
		}
		
		TextView header = (TextView)arg2.findViewById(R.id.process_id);
		header.setText(title);
		
		return arg2;
	}
	
	@Override
	public boolean hasStableIds() {
		return false;
	}
	 
	@Override
	public boolean isChildSelectable(int groupPosition, int childPosition) {
		return true;
	}
}
