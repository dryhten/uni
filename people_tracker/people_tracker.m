% Advanced Vision Assignment 1
% Mihail Dunaev
% 11 Feb 2016
%
% Program that identifies and tracks people dancing ceilidh. For
% identification it performs background subtraction and basic blob processing
% techniques, and for tracking it uses simple heuristics (based on position
% and trajectory).

height = 480; % vertical
width = 640; % horizontal
depth = 3; % RGB

% background image
bck = imread('DATA1/bgframe.jpg');

% positions of dancers in each frame
load('DATA1/positions1.mat');
first_frame = true;
num_frames = 210;
count_frames = 0;
num_dancers = 4;
unknown_dancer = 5;
dmax = 100;
area_threshold = 1500;
bd_threshold = 0.25;
my_positions = zeros(4, num_frames, 2);
n = 2;

% different color for each dancer
colors = ['r' 'y' 'g' 'b' 'w'];

% image difference threshold
eps = 10;

% dance area
da_sw = 320;
da_dw = 250;
da_sh = 60;
da_dh = 180;
dance_bck = bck(da_sh:(da_sh+da_dh), da_sw:(da_sw + da_dw), :);

% read all the images
for i = 1:num_frames
    fn = sprintf('DATA1/frame%d.jpg',i+109);
    img = imread(fn);
    
    % focus only on the dance area XXX
    dance_img = img(da_sh:(da_sh+da_dh), da_sw:(da_sw + da_dw), :);
    
    % compute the diff, take only values greater than eps
    diff_img = imabsdiff(dance_img, dance_bck);
    gray_img = (diff_img(:,:,1) > eps) | (diff_img(:,:,2) > eps) | ...
                    (diff_img(:,:,3) > eps);
    
    % morph - get rid of the noise
    gray_img = bwmorph(gray_img,'erode',2);    
    gray_img = bwmorph(gray_img,'dilate',6);

    % label each area (4 connectivity)
    labeled = bwlabel(gray_img,4);
    
    % get stats for each area; stats = {Area, Centroid, BoundingBox}
    stats = regionprops(labeled,'basic');
    
    % sort stats after area; stats(is)
    [~,is] = sort([stats.Area],'descend');
    
    % blob images
    imshow(img);
    hold on;
    
    num_blobs = min(num_dancers,size(stats,1));
    num_combined = num_dancers - num_blobs;
    pos = zeros(num_dancers, 3);
    probs = zeros(num_dancers, num_dancers);
    bds = zeros(num_dancers, num_dancers);
    blob_index = 1;
    
    % look at all the blobs
    for j = 1:num_blobs
        
        % do some precomputation on blob
        
        % blob center (height, width)
        ch = stats(is(j)).Centroid(2);
        cw = stats(is(j)).Centroid(1);
        
        % blob leftmost position
        csh = stats(is(j)).BoundingBox(2) - 0.5;
        csw = stats(is(j)).BoundingBox(1) - 0.5;
        
        % blob size
        cimgh = stats(is(j)).BoundingBox(4);
        cimgw = stats(is(j)).BoundingBox(3);
        
        % count how many pixels in blob - this is actually area
        num_elems = stats(is(j)).Area;
        
        % check if multiple dancers
        if num_combined > 0
        
            num_combined = num_combined - 1;
           
            % get xs & ys
            k = 1;
            xs = zeros(num_elems,1);
            ys = zeros(num_elems,1);
            for h = csh:(csh+cimgh)
                for w = csw:(csw+cimgw)
                    if gray_img(h,w) > 0
                        xs(k) = w;
                        ys(k) = h;
                        k = k + 1;
                    end
                end
            end

            % perform PCA, code taken from bytefish
            X = [xs ys];
            miu = mean(X);
            Xm = bsxfun(@minus, X, miu);
            C = cov(Xm);
            [V, D] = eig(C);
            
            % sort eigen
            [D, i2] = sort(diag(D),'descend');
            V = V(:,i2);
            
            % compute second line
            a = 0;
            b = 0;
            if V(1,2) ~= 0
                a = V(2,2) / V(1,2);
                b = miu(2) - (a * miu(1));
            end
            
            % separate dancers after second line; d1 = up, d2 = down;
            xs_d1 = [];
            ys_d1 = [];
            xs_d2 = [];
            ys_d2 = [];
            
            for k = 1:num_elems
                if V(1,2) == 0
                    if xs(k) < miu(1)
                        xs_d2 = [xs_d2 xs(k)];
                        ys_d2 = [ys_d2 ys(k)];
                    else
                        xs_d1 = [xs_d1 xs(k)];
                        ys_d1 = [ys_d1 ys(k)];
                    end
                else
                    line_val = xs(k) * a + b;
                    if ys(k) < line_val
                        xs_d2 = [xs_d2 xs(k)];
                        ys_d2 = [ys_d2 ys(k)];
                    else
                        xs_d1 = [xs_d1 xs(k)];
                        ys_d1 = [ys_d1 ys(k)];
                    end
                end
            end
            
            xm_d1 = mean(xs_d1);
            ym_d1 = mean(ys_d1);
            pos(blob_index,:) = [(xm_d1+da_sw) (ym_d1+da_sh) size(xs_d1,2)];
            
            % compute histogram and bhattacharyya distance for first blob
            % code take from av repo
            num_elems = size(xs_d1,2);
            histr = zeros(1, num_elems);
            histg = zeros(1, num_elems);
            histb = zeros(1, num_elems);
            
            % bins
            edges = zeros(256,1);
            for k = 1 : 256;
                edges(k) = k-1;
            end
            
            for k = 1:num_elems
                h = ys_d1(k);
                w = xs_d1(k);
                histr(k) = dance_img(h,w,1);
                histg(k) = dance_img(h,w,2);
                histb(k) = dance_img(h,w,3);
            end
            
            % one normalised vector
            histr = histc(histr,edges)';
            histr = histr / num_elems;
            histg = histc(histg,edges)';
            histg = histg / num_elems;
            histb = histc(histb,edges)';
            histb = histb / num_elems;
            
            histv = [histr' histg' histb'];
            histv = histv / 3;
            
            % save bd
            for k = 1:num_dancers
                bd = bhattacharyya(histv, hists{k});
                bds(blob_index,k) = 1 - bd;
            end
            
            % do the same for other blob
            blob_index = blob_index + 1;
            xm_d2 = mean(xs_d2);
            ym_d2 = mean(ys_d2);
            pos(blob_index,:) = [(xm_d2+da_sw) (ym_d2+da_sh) size(xs_d2,2)];

            num_elems = size(xs_d2,2);
            histr = zeros(1, num_elems);
            histg = zeros(1, num_elems);
            histb = zeros(1, num_elems);
            
            for k = 1:num_elems
                h = ys_d2(k);
                w = xs_d2(k);
                histr(k) = dance_img(h,w,1);
                histg(k) = dance_img(h,w,2);
                histb(k) = dance_img(h,w,3);
            end
            
            % one normalised vector
            histr = histc(histr,edges)';
            histr = histr / num_elems;
            histg = histc(histg,edges)';
            histg = histg / num_elems;
            histb = histc(histb,edges)';
            histb = histb / num_elems;
            
            histv = [histr' histg' histb'];
            histv = histv / 3;
            
            % save bd
            for k = 1:num_dancers
                bd = bhattacharyya(histv, hists{k});
                bds(blob_index,k) = 1 - bd;
            end
        
        else
            % save position
            pos(blob_index,:) = [(cw+da_sw) (ch+da_sh) stats(is(j)).Area];
            
            % compute hist & save bd
            histr = zeros(1, num_elems);
            histg = zeros(1, num_elems);
            histb = zeros(1, num_elems);
            
            % bins
            edges = zeros(256,1);
            for k = 1 : 256;
                edges(k) = k-1;
            end
            
            k = 1;
            for h = csh:(csh+cimgh)
                for w = csw:(csw+cimgw)
                    if gray_img(h,w) > 0
                        histr(k) = dance_img(h,w,1);
                        histg(k) = dance_img(h,w,2);
                        histb(k) = dance_img(h,w,3);
                        k = k + 1;
                    end
                end
            end
            
            % one normalised vector
            histr = histc(histr,edges)';
            histr = histr / num_elems;
            histg = histc(histg,edges)';
            histg = histg / num_elems;
            histb = histc(histb,edges)';
            histb = histb / num_elems;
        
            histv = [histr' histg' histb'];
            histv = histv / 3;
            
            % save hist if first frame
            if first_frame == true 
                dancer_index = unknown_dancer;
                if cw > (450 - da_sw)
                    if ch > (150 - da_sh)
                        dancer_index = 2;
                    else
                        dancer_index = 1;
                    end
                else
                    if ch > (150 - da_sh)
                        dancer_index = 4;
                    else
                        dancer_index = 3;
                    end
                end
                hists{dancer_index} = histv;
                probs(blob_index,dancer_index) = 1;
            else
                for k = 1:num_dancers
                    bd = bhattacharyya(histv, hists{k});
                    bds(blob_index,k) = 1 - bd;
                end
            end
        end
        
        blob_index = blob_index + 1;
    end

    if first_frame == false

        % add rgb hist prob
        probs = bds;
        
        % add everything else
        for j = 1:num_dancers
            
            % add pos prob; j = blob, k = dancer's prev position
            max_ppoz = 0;
            for k = 1:num_dancers
                dw = pos(j,1) - my_positions(k,i-1,1);
                dh = pos(j,2) - my_positions(k,i-1,2);
                dpoz = sqrt(dw*dw + dh*dh);
                ppoz = 1 - (dpoz/dmax);
                probs(j,k) = probs(j,k) * ppoz;
            end
            
            % add pred prob, j = dancer, k = blob
            if i > 2
                xs = [];
                ys = [];
                rk = 1;
                for k = 1:2
                    px = my_positions(j,i - 3 + k, 1);
                    py = my_positions(j,i - 3 + k, 2);
                    % check for duplicated xs
                    duplicated = false;
                    duplicated2 = false;
                    for k2 = 1:size(xs,2)
                        if px == xs(k2)
                            duplicated = true;
                            break;
                        end
                    end
                    
                    if duplicated == true
                        continue;
                    end;
                    
                    xs(rk) = px;
                    ys(rk) = py;
                    rk = rk + 1;
                end
               
                % interpolate - using polynomial
                if size(xs,2) == n
                    coefs = polyfit(xs, ys, n-1);
                
                    % sanity checks
                    nan_flag = false;
                    for k = 1:size(coefs,2)
                        if isinf(coefs(k))
                            nan_flag = true;
                            break;
                        end
                        if isnan(coefs(k))
                            nan_flag = true;
                            break;
                        end
                    end
        
                    if nan_flag == false

                        % compute distances, first find out max
                        errs = zeros(1,num_dancers);
                        max_err = -1;
                        for k = 1:num_dancers
                            cx = pos(k,1);
                            cy = pos(k,2);
                            err = abs(polyval(coefs,cx) - cy)/sqrt(coefs(1)*coefs(1) + 1);
                            errs(k) = err;
                            if err > max_err
                               max_err = err;
                            end
                        end
                        
                        for k = 1:num_dancers
                            probs(k,j) = probs(k,j) * power(1 - (errs(k)/(max_err+0.5)),2);
                        end 
                    end
                end  
            end            
        end        
    end
    
    % compute most probable dancer
    dancers = zeros(1,num_dancers);
    for j = 1:num_dancers
        maxp = -1;
        ki = 0;
        for k = 1:num_dancers
            if probs(k,j) > maxp
                dancers(k) = j;
                maxp = probs(k,j);
                ki = k;
            end
        end
        for k = 1:num_dancers
            probs(ki,k) = -1; 
        end
    end
    
    % save dancers
    for j = 1:num_dancers
       my_positions(dancers(j), i, :) = [pos(j,1) pos(j,2)];
    end
    
    % draw circles & centers
    for j = 1:num_dancers
        radius = sqrt(pos(j,3)/pi);
        rectangle('Position', [pos(j,1) - radius, pos(j,2) - radius, ...
            2*radius, 2*radius],'EdgeColor',colors(dancers(j)), ...
            'Curvature',[1,1]);
        plot(pos(j,1),pos(j,2),colors(dancers(j)));
    end
    
    if first_frame == true
        first_frame = false;
    end
    
    % simulate 9 frames/s = 0.11 s
    pause(0.11);
    hold off;
    count_frames = count_frames + 1;
end

% compute difference between ground truth and detected positions
num_detected = 0;
mean_distance = 0;
for i = 1:num_frames
    for dancer = 1:num_dancers
        dx = abs(positions(dancer,i,1) - my_positions(dancer,i,1));
        dy = abs(positions(dancer,i,2) - my_positions(dancer,i,2));
        d = sqrt(dx*dx + dy*dy);
        if d <= 10
           num_detected = num_detected + 1;
           mean_distance = mean_distance + d;
        end
    end
end

% display num detected, percentage and mean
mean_distance = mean_distance / num_detected;
disp(num_detected);
disp((num_detected * 100) / (num_frames * 4));
disp(mean_distance);

% draw computed trajectories on background
hold off;
imshow(bck);
hold on;

for dancer = 1:num_dancers
    plot(my_positions(dancer,:,1),my_positions(dancer,:,2),colors(dancer));
end

k = waitforbuttonpress;

% draw ground truth trajectories
for dancer = 1:num_dancers
    plot(positions(dancer,:,1),positions(dancer,:,2),colors(dancer));
end
