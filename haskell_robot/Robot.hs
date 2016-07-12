{-
 - Project 2 - Haskell robot
 - Mihail Dunaev, 325CC
 - 17 Apr 2012
 -
 - Assignment Instructions (RO) http://elf.cs.pub.ro/pp/teme2012/t2 
 -}

module Robot
where

import Types

type Tile = Int -- 0 for passable, 1 for pit

data Memory =                                   -- robot's memory representation
	Memory{
	xmax :: Int,                                -- xmax for the map
	ymax :: Int,                                -- ymax for the map
	mode :: Int,                                -- robot's mode
	route :: [Cardinal],                        -- saved route
	progressRoute :: [Cardinal],                -- robot's progress from the route
	routes :: [[Cardinal]],                     -- next routes
	exploredMap :: [(Point,Tile,SVal,Int,Int)], --  explored map
	currentPosition :: Point} deriving (Show)   -- robot's current position on the map

-- start function
startRobot (w,h) = Memory (w-1) (h-1) 0 [] [] [] [] (0,0)  

-- robot's decision - check the mode and act accordingly
perceiveAndAct signal pits (Memory xmax ymax mode route progressRoute routes exploredMap (x,y))
	
	| mode == 0 = perceiveAndAct signal pits 
						(Memory xmax ymax 1 [] [] (newRoutes pits) exploredMap (x,y))

	| mode == 1 = if null routes
					  then perceiveAndAct signal pits 
								(Memory xmax ymax 4 [] [] [] exploredMap (x,y))
				 	  else perceiveAndAct signal pits
								(Memory xmax ymax 2 (head routes) [] (tail routes) exploredMap (x,y))

	| mode == 2 = if null route
					  then perceiveAndAct signal pits 
								(Memory xmax ymax 3 (traceback1 progressRoute) [] routes (updateMap exploredMap pits (x,y) signal) (x,y))
					  else (Just (head route), 
								(Memory xmax ymax 2 (tail route) (progressRoute ++ [head route]) routes (updateMap exploredMap pits (x,y) signal) (nextPoint (x,y) (head route))))

	| mode == 3 = if null route
					  then perceiveAndAct signal pits 
								(Memory xmax ymax 1 [] [] routes (updateMapFast exploredMap (x,y) signal pits) (x,y))
					  else (Just (head route), 
								(Memory xmax ymax 3 (tail route) [] routes (updateMapFast exploredMap (x,y) signal pits) (nextPoint (x,y) (head route))))

	| mode == 4 = if (maxUnexploredMap exploredMap) == 0
					  then (Nothing, (Memory xmax ymax mode route progressRoute routes exploredMap (x,y))) -- get back to base
					  else perceiveAndAct signal pits 
										(Memory xmax ymax 5 (getRouteToUnexploredMap (x,y) exploredMap (maxUnexploredMap exploredMap)) [] [] exploredMap (x,y))

	| mode == 5	= if null route -- end of road
					  then perceiveAndAct signal pits 
								(Memory xmax ymax 0 [] [] [] (updateDestination exploredMap (x,y)) (x,y))
					  else (Just (head route),
								(Memory xmax ymax 5 (tail route) [] [] exploredMap (nextPoint (x,y) (head route))))


-- possible routes for the robot
newRoutes pits = 
	plainToDeep (nextPossibleMoves pits)

-- checks for non-pits near the robot
nextPossibleMoves pits = 
	setDifference [N,S,E,W] pits

-- math function for set difference (a,b = lists)
setDifference a b = filter (\x -> if elem x b then False else True) a

-- creates a new list for each element of the original list
plainToDeep [] = []
plainToDeep (x:xs) = [x]:(plainToDeep xs)

-- function that computes the reverse route for 'route'
traceback1 route = traceback (reverse route) 
traceback [] = []
traceback (N:route) = S:(traceback route)
traceback (S:route) = N:(traceback route)
traceback (W:route) = E:(traceback route)
traceback (E:route) = W:(traceback route) 

-- updates robot's explored map
updateMap exploredMap pits (x,y) signal = 
	if mapContainsPoint (x,y) exploredMap 
	then updateSignalAndUnexploredAt (x,y) exploredMap exploredMap signal pits
	else addPitsToMap pits 
				(((x,y),0,signal,
					(4-(length pits)-(magicNumber [(x+1,y),(x,y+1),(x-1,y),(x,y-1)] exploredMap)),0):exploredMap) (x,y)

-- updates only the signal
updateMapFast exploredMap (x,y) signal pits = updateSignalAndUnexploredAt (x,y) exploredMap exploredMap signal pits

-- function that checks if the robot knows about (x,y)
mapContainsPoint (x,y) [] = False
mapContainsPoint (x,y) (((c1,c2),t,s,l,d):exploredMap) = 
	if x == c1 && y == c2 then True else mapContainsPoint (x,y) exploredMap

-- function that checks how many squares near (x,y) have been explored and are not pits
magicNumber [] exploredMap = 0
magicNumber ((a,b):lst) exploredMap = 
	if mapContainsPassable (a,b) exploredMap 
	then 1 + magicNumber lst exploredMap
	else magicNumber lst exploredMap

-- function that checks if (x,y) has been explored and is passable
mapContainsPassable (x,y) [] = False
mapContainsPassable (x,y) (((c1,c2),t,s,l,d):exploredMap) = 
	if x == c1 && y == c2 && t == 0 then True else mapContainsPassable (x,y) exploredMap

-- functions that updates signal value at (x,y)
updateSignalAt (x,y) [] signal = []
updateSignalAt (x,y) (((c1,c2),t,s,l,d):exploredMap) signal =
	if x == c1 && y == c2 
	then ((c1,c2),t,signal,l,d):exploredMap
	else ((c1,c2),t,s,l,d):(updateSignalAt (x,y) exploredMap signal)

updateDestination [] (x,y) = []
updateDestination (((c1,c2),t,s,l,d):exploredMap) (x,y) = 
	if x == c1 && y == c2 
	then ((c1,c2),t,s,l,1):exploredMap
	else ((c1,c2),t,s,l,d):(updateDestination exploredMap (x,y))

updateSignalAndUnexploredAt (x,y) [] auxExploredMap signal pits = []
updateSignalAndUnexploredAt (x,y) (((c1,c2),t,s,l,d):exploredMap) auxExploredMap signal pits =
	if x == c1 && y == c2
	then ((c1,c2),t,signal,(4-(length pits)-(magicNumber [(x+1,y),(x,y+1),(x-1,y),(x,y-1)] auxExploredMap)),d):exploredMap
	else ((c1,c2),t,s,l,d):(updateSignalAndUnexploredAt (x,y) exploredMap auxExploredMap signal pits)

-- function that adds pits positions to explored map
addPitsToMap [] exploredMap (x,y) = exploredMap
addPitsToMap (N:pits) exploredMap (x,y) = addPitsToMap pits (((x,y-1),1,0,0,1):exploredMap) (x,y)
addPitsToMap (S:pits) exploredMap (x,y) = addPitsToMap pits (((x,y+1),1,0,0,1):exploredMap) (x,y)
addPitsToMap (E:pits) exploredMap (x,y) = addPitsToMap pits (((x+1,y),1,0,0,1):exploredMap) (x,y)
addPitsToMap (W:pits) exploredMap (x,y) = addPitsToMap pits (((x-1,y),1,0,0,1):exploredMap) (x,y)

-- function that returns the maximum signal on the map
maxSignal exploredMap = maxSignalAcc exploredMap 0
maxSignalAcc [] acc = acc
maxSignalAcc (((c1,c2),t,s,l,d):exploredMap) acc = 
	if s > acc && d == 0 
	then maxSignalAcc exploredMap s
	else maxSignalAcc exploredMap acc

-- function that computes the "unexplored value" for the map
maxUnexploredMap exploredMap = maxUnexploredMapAcc exploredMap 0
maxUnexploredMapAcc [] acc = acc
maxUnexploredMapAcc (((c1,c2),t,s,l,d):exploredMap) acc = 
	if l > acc
	then maxUnexploredMapAcc exploredMap l
	else maxUnexploredMapAcc exploredMap acc

-- function that returns (x,y) for the square with maximum signal
maxSignalPosition maxS (((c1,c2),t,s,l,d):exploredMap) =  
	if s == maxS then (c1,c2) else maxSignalPosition maxS exploredMap

-- function that returns (x,y) for the square with largest "unexplored value"
maxUPosition maxU (((c1,c2),t,s,l,d):exploredMap) = 
	if l == maxU then (c1,c2) else maxUPosition maxU exploredMap

-- next (x,y) using N,S,E,W values
nextPoint (x,y) N = (x,y-1)
nextPoint (x,y) S = (x,y+1)
nextPoint (x,y) E = (x+1,y)
nextPoint (x,y) W = (x-1,y)

-- function that computes fastest route from (x,y) to (xf,yf) using bfs
getRouteFromTo (x,y) (xf,yf) exploredMap = checkIfDone [[(x,y)]] (xf,yf) exploredMap

-- bfs helper function
makeRoutes routes acc exploredMap (xf,yf) =
	if null routes
	then checkIfDone acc (xf,yf) exploredMap
	else makeRoutes (tail routes) ((onlyNewRoutes (getAllPossibleRoutes (head routes) exploredMap) acc) ++ acc) exploredMap (xf,yf)

-- function that creates all possible new routes from route
getAllPossibleRoutes ((x,y):route) exploredMap = getAllPossibleRoutes2 [(x+1,y),(x,y+1),(x-1,y),(x,y-1)] ((x,y):route) exploredMap
getAllPossibleRoutes2 [] route exploredMap = []
getAllPossibleRoutes2 ((a,b):list) route exploredMap = 
	if mapContainsPassable (a,b) exploredMap
	then ((a,b):route):(getAllPossibleRoutes2 list route exploredMap)
	else getAllPossibleRoutes2 list route exploredMap

-- function that filters all the routes generated with bfs
onlyNewRoutes [] acc = []
onlyNewRoutes routes acc = 
	if containsRoute acc (head routes)
	then onlyNewRoutes (tail routes) acc
	else (head routes):(onlyNewRoutes (tail routes) acc)

-- function that checks if a list of routes has two with same destination
containsRoute [] r = False
containsRoute (((x,y):route):routes) ((a,b):r) =
	if a == x && b == y then True
	else containsRoute routes ((a,b):r)

-- control function
checkIfDone routes (xf,yf) exploredMap = 
	if weDone routes (xf,yf)
	then returnRoute routes (xf,yf)
	else makeRoutes routes [] exploredMap (xf,yf)

-- function that checks if we reached the final square
weDone [] (xf,yf) = False
weDone (((x,y):route):routes) (xf,yf) = 
	if x == xf && y == yf then True else weDone routes (xf,yf)

-- function that returns best route
returnRoute [] (xf,yf) = []
returnRoute (((x,y):route):routes) (xf,yf) =
	if x == xf && y == yf then ((x,y):route) else returnRoute routes (xf,yf)

-- function that converts route to list of directions
convertFromPointsToDirections [(x,y)] = []
convertFromPointsToDirections ((x,y):(a,b):list)
	| a == (x + 1) && b == y = E:convertFromPointsToDirections ((a,b):list)
	| a == x && b == (y + 1) = S:convertFromPointsToDirections ((a,b):list)
	| a == (x - 1) && b == y = W:convertFromPointsToDirections ((a,b):list)
	| a == x && b == (y - 1) = N:convertFromPointsToDirections ((a,b):list)

getRouteToMaxSignal (x,y) exploredMap maxS = 
	convertFromPointsToDirections (reverse (getRouteFromTo (x,y) (maxSignalPosition maxS exploredMap) exploredMap))

getRouteToUnexploredMap (x,y) exploredMap maxU = 
	convertFromPointsToDirections (reverse (getRouteFromTo (x,y) (maxUPosition maxU exploredMap) exploredMap))