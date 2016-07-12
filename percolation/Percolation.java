/**
 * @author Mihail Dunaev
 * 28 Aug 2012
 * 
 * Java program that deals with the percolation problem : 
 * 
 * Percolation :
 * Given a composite systems comprised of randomly distributed insulating and metallic materials: what 
 * fraction of the materials need to be metallic so that the composite system is an electrical conductor? 
 * Given a porous landscape with water on the surface (or oil below), under what conditions will the water
 * be able to drain through to the bottom (or the oil to gush through to the surface)? Scientists have defined 
 * an abstract process known as percolation to model such situations.
 * 
 * The model used : 
 * N-by-N grid of sites. Each site is either open or blocked. A full site is an open site that 
 * can be connected to an open site in the top row via a chain of neighboring (left, right, up, down) open sites. 
 * We say the system percolates if there is a full site in the bottom row. In other words, a system percolates if 
 * we fill all open sites connected to the top row and that process fills some open site on the bottom row.
 * (For the insulating/metallic materials example, the open sites correspond to metallic materials, so that 
 * a system that percolates has a metallic path from top to bottom, with full sites conducting. 
 * For the porous substance example, the open sites correspond to empty space through which water might flow, 
 * so that a system that percolates lets water fill open sites, flowing from top to bottom.)  
 * 
 * The problem :
 * In a famous scientific problem, researchers are interested in the following question: 
 * if sites are independently set to be open with probability p (and therefore blocked with probability 1 - p), 
 * what is the probability that the system percolates? When p equals 0, the system does not percolate; when p equals 1, 
 * the system percolates. The plots below show the site vacancy probability p versus the percolation probability 
 * for 20-by-20 random grid (left) and 100-by-100 random grid (right). 
 * When N is sufficiently large, there is a threshold value p* such that when p < p* a random 
 * N-by-N grid almost never percolates, and when p > p*, a random N-by-N grid almost always percolates. 
 * No mathematical solution for determining the percolation threshold p* has yet been derived. 
 * The task is to write a computer program to estimate p*. 
 * 
 * Some sort of interface :
 * public Percolation (int N) = creates a new N-by-N grid of sites (object), with each site initially blocked
 * public void open (int i, int j) = opens site at (i,j)
 * public boolean isOpen (int i, int j) = checks if the site at (i,j) is either empty or full
 * public boolean isFull (int i, int j) = checks if site at (i,j) is full
 * public boolean percolates () = checks if the system percolates
 * 
 * Last modification : 28 Aug 2012
 * 
 */

public class Percolation {

	private enum site{ 
		blocked,
		empty,
		full
	};
	
	private WeightedQuickUnionUF uf; 
	private site map[][];
	private int size, source, dest; // source and destination = 2 extra nodes
	
	public Percolation (int N){
		
		int mapsize = N*N;
		map = new site[N][N];
		size = N;
		source = mapsize; // union starts from 0
		dest = mapsize + 1;
		uf = new WeightedQuickUnionUF(mapsize + 2);
		
		for (int i = 0; i < N; i++)
			for (int j = 0; j < N; j++)
				map[i][j] = site.blocked;
	}
	
	private void updateSites (int i, int j){ // private method that will update empty sites to full if necessary ( i.e. a "full" chain connects with an "empty" one )
		
		map[i][j] = site.full;
		
		if ((i != 0) && (map[i-1][j] == site.empty)) // up
			updateSites(i-1,j);
		if ((i != (size-1)) && (map[i+1][j] == site.empty)) // down
			updateSites(i+1,j);
		if ((j != 0) && (map[i][j-1] == site.empty)) // left
			updateSites(i,j-1);
		if ((j != (size-1)) && (map[i][j+1] == site.empty)) // right
			updateSites(i,j+1);
	}
	
	public void open (int i, int j){
		
		// resetting indexes
		i = i - 1; 
		j = j - 1;
		
		// correct positions?
		if (i < 0 || i >= size || j < 0 || j >= size)
			throw new java.lang.IndexOutOfBoundsException();
		
		// if it's already opened / full don't do anything; else
		if (map[i][j] == site.blocked){  
			
			map[i][j] = site.empty;
			
			// neighbors
			int unionpos = i*size + j;
			int npos, auxi = i + 1, auxj = j + 1;
			
			// up
			if (i == 0){
				map[i][j] = site.full;
				uf.union(source, unionpos);
			}
			else if (isFull (auxi-1,auxj)){
				map[i][j] = site.full;
				npos = (i-1)*size + j;
				uf.union(npos, unionpos);
			}
			else if (isOpen(auxi-1,auxj)){
				npos = (i-1)*size + j;
				uf.union(npos, unionpos);
			}

			// down
			if (i == (size-1)){
				uf.union(dest, unionpos);
			}
			else if (isFull(auxi+1,auxj)){
				map[i][j] = site.full;
				npos = (i+1)*size + j;
				uf.union(npos, unionpos);
			}
			else if (isOpen(auxi+1,auxj)){
				npos = (i+1)*size + j;
				uf.union(npos, unionpos);
			}
			
			// left
			if (j != 0){
				if (isFull(auxi,auxj-1)){
					map[i][j] = site.full;
					npos = unionpos - 1;
					uf.union(npos, unionpos);
				}
				else if (isOpen(auxi,auxj-1)){
					npos = unionpos - 1;
					uf.union(npos, unionpos);
				}
			}
			
			// right
			if (j != (size-1)){
				if (isFull(auxi,auxj+1)){
					map[i][j] = site.full;
					npos = unionpos + 1;
					uf.union(npos, unionpos);
				}
				else if (isOpen(auxi,auxj+1)){
					npos = unionpos + 1;
					uf.union(npos, unionpos);
				}
			}
			
			// update empty neighbors cells if full
			if (map[i][j] == site.full){
				if ((i != 0) && (map[i-1][j] == site.empty)) // up
					updateSites(i-1,j);
				if ((i != (size-1)) && (map[i+1][j] == site.empty)) // down
					updateSites(i+1,j);
				if ((j != 0) && (map[i][j-1] == site.empty)) // left
					updateSites(i,j-1);
				if ((j != (size-1)) && (map[i][j+1] == site.empty)) // right
					updateSites(i,j+1);
			}
		}
	}
	
	public boolean isOpen (int i, int j){ // will check if site is at least empty (empty or full) 
		
		i = i - 1; // starts from 0
		j = j - 1;
		
		if (i < 0 || i >= size || j < 0 || j >= size)
			throw new java.lang.IndexOutOfBoundsException();
		
		if (map[i][j] == site.empty || map[i][j] == site.full) // only empty ?
			return true;
		
		return false;
	}
	
	public boolean isFull (int i, int j){
		
		i = i - 1; // starts from 0
		j = j - 1;
		
		if (i < 0 || i >= size || j < 0 || j >= size)
			throw new java.lang.IndexOutOfBoundsException();
		
		if (map[i][j] == site.full) 
			return true;
		
		return false;
	}
	
	public boolean percolates (){
		return uf.connected(source, dest);
	}
}
