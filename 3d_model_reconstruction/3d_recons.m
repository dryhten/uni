% Advanced Vision Assignment 2
% Mihail Dunaev
% 17 March 2016
%
% Program that will reconstruct the 3D model of an object with 9 planes, from
% images (that contain both RGB and depth values) taken from different angles.

% load data
load('av_pcl.mat');

% some initializations
A = zeros(3,3);
B = zeros(3,3);

% iterate through images
nimgs = 16;
cmap = hsv(nimgs);
for i = 1:nimgs
    img = pcl_cell{i};
    
    % rgb and depth
    img_rgb = img(:,:,1:3);
    img_depth = img(:,:,4:6);
    
    % reshape XYZ image as 1D array (col major)
    img_size = size(img_depth,1) * size(img_depth,2);
    img_depth_array = reshape(img_depth,[img_size,3]);
    
    % plot points before extraction
    if i == 1
        plot3(img_depth_array(:,1),img_depth_array(:,2),img_depth_array(:,3),'.b');
        while true
            k = waitforbuttonpress;
            if k ~= 0 
                break;
            end
        end
    end
    
    % plane = plane_pts, objects = obj_pts
    
    % get 4 points for plane
    plane_start_pts = zeros(4,3);
    plane_start_pts(1,:) = img_depth_array(99*480 + 100,:);
    plane_start_pts(2,:) = img_depth_array(540*480 + 100,:);
    plane_start_pts(3,:) = img_depth_array(99*480 + 381,:);
    plane_start_pts(4,:) = img_depth_array(99*480 + 100,:);

    % remove pts from objects data
    obj_pts = img_depth_array;
    obj_pts(540*480 + 381,:) = [];
    obj_pts(540*480 + 100,:) = [];
    obj_pts(99*480 + 381,:) = [];
    obj_pts(540*480 + 381,:) = [];
    
    % fit plane
    [plane, r] = fitplane(plane_start_pts);
    if r >= 0.1
        fprintf('Could not find fit plane from the given points\n');
        return;
    end
    
    % get all the points from the plane
    plane_pts = zeros(img_size,3);
    plane_pts(1:4,:) = plane_start_pts;
    
    % transfer points from obj_pts to plane_pts
    eps_plane = 0.01;
    j = 1;
    num_pts = 4;
    tmp_size = img_size - 4;
    while true
        if j > tmp_size
            break;
        end
        pnt = obj_pts(j,:);
        
        % first check if point is beyond the plane
        z_plane = (-plane(4) - plane(1) * pnt(1) - plane(2) * pnt(2))/plane(3);
        if pnt(3) > z_plane
            num_pts = num_pts + 1;
            plane_pts(num_pts,:) = obj_pts(j,:);
            obj_pts(j,:) = [0,0,0];
            j = j + 1;
            continue;
        end
        
        % check if within certain distance (eps)
        dist_plane = abs([obj_pts(j,:) 1]*plane);
        if dist_plane < eps_plane
            num_pts = num_pts + 1;
            plane_pts(num_pts,:) = obj_pts(j,:);
            obj_pts(j,:) = [0,0,0];
        end
        j = j + 1;
    end
    
    % get rid of the 0 elements
    cnt = 0;
    for j = 1:size(obj_pts,1)
        if (obj_pts(j,1) == 0) && (obj_pts(j,2) == 0) && ...
                (obj_pts(j,3) == 0)
            continue;
        end
        cnt = cnt + 1;
    end
    
    new_obj_pts = zeros(cnt,3);
    colors = zeros(cnt,3);
    cnt = 0;
    for j = 1:size(obj_pts,1)
        if (obj_pts(j,1) == 0) && (obj_pts(j,2) == 0) && ...
                (obj_pts(j,3) == 0)
            continue;
        end
        cnt = cnt + 1;
        new_obj_pts(cnt,:) = obj_pts(j,:);
        [x,y] = map_index_to_pos(j);
        colors(cnt,:) = img_rgb(y,x,:);
    end
    
    % plot points after extraction
    if i == 1
        plot3(new_obj_pts(:,1),new_obj_pts(:,2),new_obj_pts(:,3),'.b');
        while true
            k = waitforbuttonpress;
            if k ~= 0 
                break;
            end
        end
    end
    
    % cluster regions using dbscan
    [la, type] = dbscan(new_obj_pts,5,[]);
    
    % find max
    max_la = -1;
    sz = size(new_obj_pts,1);

    for j = 1:sz
        if la(j) > max_la
            max_la = la(j);
        end
    end
    
    % count how many for each cluster
    cnts = zeros(max_la,1);
    for j = 1:sz
        if la(j) > 0
            cnts(la(j)) = cnts(la(j)) + 1;
        end
    end
    
    % sort descending and get first 4 clusters
    [~,is] = sort(cnts,'descend');
    
    obj1 = zeros(cnts(is(1)),3);
    colors_obj1 = zeros(cnts(is(1)),3);
    obj2 = zeros(cnts(is(2)),3);
    obj3 = zeros(cnts(is(3)),3);
    obj4 = zeros(cnts(is(4)),3);
    
    cnt1 = 0;
    cnt2 = 0;
    cnt3 = 0;
    cnt4 = 0;

    for j = 1:sz
        if la(j) == is(1)
            cnt1 = cnt1 + 1;
            obj1(cnt1,:) = new_obj_pts(j,:);
            colors_obj1(cnt1,:) = colors(j,:);
        end
        if la(j) == is(2)
            cnt2 = cnt2 + 1;
            obj2(cnt2,:) = new_obj_pts(j,:);
        end
        if la(j) == is(3)
            cnt3 = cnt3 + 1;
            obj3(cnt3,:) = new_obj_pts(j,:);
        end
        if la(j) == is(4)
            cnt4 = cnt4 + 1;
            obj4(cnt4,:) = new_obj_pts(j,:);
        end
    end
    
    % fit sphere for each ball & get center
    [cobj2,robj2] = sphereFit(obj2);
    [cobj3,robj3] = sphereFit(obj3);
    [cobj4,robj4] = sphereFit(obj4);
    
    % optional - plot each object with a diff color
    if i == 1
        figure;
        hold on;
        plot3(obj1(:,1),obj1(:,2),obj1(:,3),'.b');
        plot3(obj2(:,1),obj2(:,2),obj2(:,3),'.r');
        plot3(obj3(:,1),obj3(:,2),obj3(:,3),'.g');
        plot3(obj4(:,1),obj4(:,2),obj4(:,3),'.y');
        while true
            k = waitforbuttonpress;
            if k ~= 0
                break;
            end
        end
        
        [base_x,base_y,base_z] = sphere(20);
        surf(robj2*base_x+cobj2(1), robj2*base_y+cobj2(2), robj2*base_z+cobj2(3),...
        'faceAlpha', 0.3, 'Facecolor', 'c');
        [base_x,base_y,base_z] = sphere(20);
        surf(robj3*base_x+cobj3(1), robj3*base_y+cobj3(2), robj3*base_z+cobj3(3),...
        'faceAlpha', 0.3, 'Facecolor', 'c');
        [base_x,base_y,base_z] = sphere(20);
        surf(robj4*base_x+cobj4(1), robj4*base_y+cobj4(2), robj4*base_z+cobj4(3),...
        'faceAlpha', 0.3, 'Facecolor', 'c');
        while true
            k = waitforbuttonpress;
            if k ~= 0
                hold off;
                break;
            end
        end
    end
    
    % go back to 2D image to compute histogram
    lbls = zeros(img_size,1);
    
    % look for occurence of first element from obj2
    [~, locb] = my_ismember(obj2(1,:),obj_pts);
    lbls(locb) = 2;

    cnt = size(obj2,1);
    for k = 2:cnt
        while (locb < img_size)
            locb = locb + 1;
            if obj_pts(locb,1) ~= obj2(k,1)
                continue;
            end
            if obj_pts(locb,2) ~= obj2(k,2)
                continue;
            end
            if obj_pts(locb,3) ~= obj2(k,3)
                continue;
            end
            break;
        end
        lbls(locb) = 2;
    end

    % look for occurence of first element from obj3
    [~, locb] = my_ismember(obj3(1,:),obj_pts);
    lbls(locb) = 3;

    cnt = size(obj3,1);
    for k = 2:cnt
        while (locb < img_size)
            locb = locb + 1;
            if obj_pts(locb,1) ~= obj3(k,1)
                continue;
            end
            if obj_pts(locb,2) ~= obj3(k,2)
                continue;
            end
            if obj_pts(locb,3) ~= obj3(k,3)
                continue;
            end
            break;
        end
        lbls(locb) = 3;
    end

    % look for occurence of first element from obj3
    [~, locb] = my_ismember(obj4(1,:),obj_pts);
    lbls(locb) = 4;

    cnt = size(obj4,1);
    for k = 2:cnt
        while (locb < img_size)
            locb = locb + 1;
            if obj_pts(locb,1) ~= obj4(k,1)
                continue;
            end
            if obj_pts(locb,2) ~= obj4(k,2)
                continue;
            end
            if obj_pts(locb,3) ~= obj4(k,3)
                continue;
            end
            break;
        end
        lbls(locb) = 4;
    end
    
    % compute histogram for each ball
    % obj2
    obj2_sz = size(obj2,1);
    histr2 = zeros(1, obj2_sz);
    histg2 = zeros(1, obj2_sz);
    histb2 = zeros(1, obj2_sz);

    % obj3
    obj3_sz = size(obj3,1);
    histr3 = zeros(1, obj3_sz);
    histg3 = zeros(1, obj3_sz);
    histb3 = zeros(1, obj3_sz);

    % obj4
    obj4_sz = size(obj4,1);
    histr4 = zeros(1, obj4_sz);
    histg4 = zeros(1, obj4_sz);
    histb4 = zeros(1, obj4_sz);

    k2 = 1;
    k3 = 1;
    k4 = 1;
    for j = 1:img_size
        if lbls(j) == 2
            [x,y] = map_index_to_pos(j);
            histr2(k2) = img_rgb(y,x,1);
            histg2(k2) = img_rgb(y,x,2);
            histb2(k2) = img_rgb(y,x,3);
            k2 = k2 + 1;
        end
        if lbls(j) == 3
            [x,y] = map_index_to_pos(j);
            histr3(k3) = img_rgb(y,x,1);
            histg3(k3) = img_rgb(y,x,2);
            histb3(k3) = img_rgb(y,x,3);
            k3 = k3 + 1;
        end
        if lbls(j) == 4
            [x,y] = map_index_to_pos(j);
            histr4(k4) = img_rgb(y,x,1);
            histg4(k4) = img_rgb(y,x,2);
            histb4(k4) = img_rgb(y,x,3);
            k4 = k4 + 1;
        end
    end

    % bins
    edges = zeros(256,1);
    for k = 1 : 256;
        edges(k) = k-1;
    end

    % one normalised vector

    % obj2
    histr2 = histc(histr2,edges)';
    histr2 = histr2 / obj2_sz;
    histg2 = histc(histg2,edges)';
    histg2 = histg2 / obj2_sz;
    histb2 = histc(histb2,edges)';
    histb2 = histb2 / obj2_sz;

    histv2 = [histr2' histg2' histb2'];
    histv2 = histv2 / 3;

    % obj3
    histr3 = histc(histr3,edges)';
    histr3 = histr3 / obj3_sz;
    histg3 = histc(histg3,edges)';
    histg3 = histg3 / obj3_sz;
    histb3 = histc(histb3,edges)';
    histb3 = histb3 / obj3_sz;

    histv3 = [histr3' histg3' histb3'];
    histv3 = histv3 / 3;

    % obj4
    histr4 = histc(histr4,edges)';
    histr4 = histr4 / obj4_sz;
    histg4 = histc(histg4,edges)';
    histg4 = histg4 / obj4_sz;
    histb4 = histc(histb4,edges)';
    histb4 = histb4 / obj4_sz;

    histv4 = [histr4' histg4' histb4'];
    histv4 = histv4 / 3;
    
    % if first image, save histograms
    if i == 1
        A = [cobj2; cobj3; cobj4];
        base_hist_2 = histv2;
        base_hist_3 = histv3;
        base_hist_4 = histv4;
        base_obj_pts = obj1;
        continue;
    end
    
    % find out which object is which (2,3,4 from first image),
    % put them in the same order
    % obj2 from first image
    bd2 = bhattacharyya(base_hist_2, histv2);
    bd3 = bhattacharyya(base_hist_2, histv3);
    bd4 = bhattacharyya(base_hist_2, histv4);
    if bd2 < bd3
        if bd2 < bd4
            B(1,:) = cobj2;
        else
            B(1,:) = cobj4;
        end
    else
        if bd3 < bd4
            B(1,:) = cobj3;
        else
            B(1,:) = cobj4;
        end
    end

    % obj3 from first image
    bd2 = bhattacharyya(base_hist_3, histv2);
    bd3 = bhattacharyya(base_hist_3, histv3);
    bd4 = bhattacharyya(base_hist_3, histv4);
    if bd2 < bd3
        if bd2 < bd4
            B(2,:) = cobj2;
        else
            B(2,:) = cobj4;
        end
    else
        if bd3 < bd4
            B(2,:) = cobj3;
        else
            B(2,:) = cobj4;
        end
    end

    % obj4 from first image
    bd2 = bhattacharyya(base_hist_4, histv2);
    bd3 = bhattacharyya(base_hist_4, histv3);
    bd4 = bhattacharyya(base_hist_4, histv4);
    if bd2 < bd3
        if bd2 < bd4
            B(3,:) = cobj2;
        else
            B(3,:) = cobj4;
        end
    else
        if bd3 < bd4
            B(3,:) = cobj3;
        else
            B(3,:) = cobj4;
        end
    end
    
    % rotate and translate
    [R,t] = rigid_transform_3D(B,A);
    transf_obj1 = R * obj1' + repmat(t, 1, size(obj1,1));
    transf_obj1 = transf_obj1';
    
    % add to the base obj points
    base_obj_pts = union(base_obj_pts, transf_obj1, 'rows');
end

% plot combined points
plot3(base_obj_pts(:,1),base_obj_pts(:,2),base_obj_pts(:,3),'.b');
while true
    k = waitforbuttonpress;
    if k ~= 0
        break;
    end
end

eps = 0.01;
eps_plane = 0.01;
start_num_pts = 100;
aux_obj_pts = base_obj_pts;

% base_obj_pts --> tmp_plane_pts
num_planes = 0;
planes = zeros(4,9);

figure;
hold on;

while num_planes < 9

    sz = size(aux_obj_pts,1);
    % make sure we have enough pts for a plane
    if sz < 1000
        break;
    end
    
    tmp_plane_pts = zeros(sz,3);

    % get random point from the plane
    pnt_index = floor(rand(1,1) * sz);
    if pnt_index < 1
        pnt_index = 1;
    end
    pnt = aux_obj_pts(pnt_index,:);

    % add it to plane
    tmp_plane_pts(1,:) = pnt;
    aux_obj_pts(pnt_index,:) = [];
    sz = sz - 1;

    % get 9 other pts close by
    % get all the points within eps distance
    num_pts = 1;
    i = 1;
    while true
        if i > sz
            break;
        end
        dist = norm(aux_obj_pts(i,:) - pnt);
        if dist < eps
            num_pts = num_pts + 1;
            tmp_plane_pts(num_pts,:) = aux_obj_pts(i,:);
            aux_obj_pts(i,:) = [];
            sz = sz - 1;
            continue;
            %if num_pts > (start_num_pts - 1)
            %    break;
            %end
        end
        i = i + 1;
    end
    
    % get rid of noise
    if num_pts < 100
        continue;
    end
    
    first = true;
    first_r = 0;
    tmp_plane = zeros(4,1);
    while true

        added = 0;

        % try to fit a plane
        [tmp_plane, r] = fitplane(tmp_plane_pts(1:num_pts,:));
        if first == true
            first_r = r;
            first = false;
        end
        if r >= 0.1
            % just noise
            break;
        end

        % add all the pts within eps_plane and eps to other pts
        for i = 1:size(aux_obj_pts,1)
            pnt = aux_obj_pts(i,:);
            dist_plane = abs([pnt 1]*tmp_plane);
            if dist_plane < eps_plane
                for k = num_pts:-1:1
                    dist = norm(tmp_plane_pts(k,:) - pnt);
                    if dist < eps
                        added = added + 1;
                        num_pts = num_pts + 1;
                        tmp_plane_pts(num_pts,:) = pnt;
                        aux_obj_pts(i,:) = [-10,-10,-10];
                        break;
                    end
                end
            end
        end

        % get rid of the -10 values from base_obj_pts
        cnt = 0;
        tmp_bop = zeros(sz - num_pts,3);
        for i = 1:size(aux_obj_pts,1)
            if aux_obj_pts(i,:) == [-10,-10,-10]
                continue;
            end
            cnt = cnt + 1;
            tmp_bop(cnt,:) = aux_obj_pts(i,:);
        end

        aux_obj_pts = tmp_bop;
        % if we got less than 50 new points, no point to go on
        if added < 50
            break;
        end
    end

    % if we got less than 5k pts, not a plane
    if num_pts < 1000
        continue;
    end
    
    num_planes = num_planes + 1;
    planes(:,num_planes) = tmp_plane;
    
    % compute z value (plane) for each point (or just 100 of them)
    plane_pts = zeros(size(tmp_plane_pts,1),3);
    for k = 1:size(tmp_plane_pts,1)
        x = tmp_plane_pts(k,1);
        y = tmp_plane_pts(k,2);
        plane_pts(k,1) = x;
        plane_pts(k,2) = y;
        plane_pts(k,3) = (-planes(4,num_planes) - planes(1,num_planes) * x - planes(2,num_planes) * y)/planes(3,num_planes);
    end
    
    % plot plane with a diff color
    clf;
    plot3(aux_obj_pts(:,1),aux_obj_pts(:,2),aux_obj_pts(:,3),'.b');
    hold on;
    plot3(plane_pts(:,1),plane_pts(:,2),plane_pts(:,3),'.', 'Color', cmap(num_planes,:));
    fprintf('first r = %f\n', first_r);
    while true
        k = waitforbuttonpress;
        if k ~= 0 
            break;
        end
    end
end