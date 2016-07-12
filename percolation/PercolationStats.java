/**
 * @author Mihail Dunaev
 * 28 Aug 2012
 * 
 * Java program that computes statistics for "Percolation.java"
 * 
 * It uses Monte Carlo simulation to estimate the threshold.
 * public PercolationStats (int N, int T) = performs T independent computational experiments on an N-by-N grid and saves the results ( estimated probability for each test )
 * public double mean () = computes the mean of percolation threshold, saves the value ( for later use ) and returns it  
 * public double stddev () = computes the standard deviation of percolation threshold, saves the value ( for later use ) and returns it
 * public static void main(String[] args) = test client; "java PercolationStats N T" will compute and print to stdout the mean, stddev and the 95% confidence interval for the T independet test on the N-by-N grid
 * 
 * Last modification : 28 Aug 2012
 */
public class PercolationStats {

	private int T;
	private double fractions[];
	private double mean, stddev;
	
	public PercolationStats (int N, int T){
		
		if (N <= 0 || T <= 0)
			throw new java.lang.IllegalArgumentException();
		
		int[] blockedpos = new int[N*N]; // used to pick a random blocked site
		fractions = new double[T];
		int size = N*N;
		this.T = T;
		
		for (int i = 0; i < size; i++)
			blockedpos[i] = i;
		
		for (int i = 0; i < T; i++){
			Percolation p = new Percolation(N);
			int opens = 0, closeds = size;
			
			while (p.percolates() == false){
				int randomIndex = StdRandom.uniform(closeds-1);
				int randomPosition = blockedpos[randomIndex];
				p.open((randomPosition/N)+1, (randomPosition%N)+1);
				
				// swap the position from randomIndex with last one from blockedpos[] so we won't use it again
				blockedpos[randomIndex] = blockedpos[randomIndex] ^ blockedpos[closeds-1];
				blockedpos[closeds-1] = blockedpos[randomIndex] ^ blockedpos[closeds-1];
				blockedpos[randomIndex] = blockedpos[randomIndex] ^ blockedpos[closeds-1];
				opens++;
				closeds--;
			}
			fractions[i] = (double)opens / size;
		}
	}

	public double mean (){
		mean = 0;		
		for (int i = 0; i < T; i++)
			mean = mean + fractions[i];
		mean = mean / T;
		return mean;
	}
	
	public double stddev (){
		
		stddev = 0;
		
		for (int i = 0; i < T; i++){
			double diff = (fractions[i] - mean);
			stddev =  stddev + (diff * diff);
		}
		
		stddev = stddev / T;
		stddev = Math.sqrt(stddev);
		return stddev;
	}

	public static void main(String[] args) {
		
		int N = 200;//Integer.parseInt(args[0]);
		int T = 100;//Integer.parseInt(args[1]);
		
		PercolationStats ps = new PercolationStats(N,T);
		double mean = ps.mean();
		double stddev = ps.stddev();
		System.out.println("mean = " + mean);
		System.out.println("stddev = " + stddev);
		
		double aux = (1.96 * stddev) / Math.sqrt(T);
		double lowerlimit = mean - aux;
		double upperlimit = mean + aux;
		System.out.printf("95%% confidence interval = %f, %f", lowerlimit, upperlimit);
	}
}
