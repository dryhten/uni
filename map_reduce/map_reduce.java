import java.text.DecimalFormat;
import java.util.*;
import java.util.concurrent.*;
import java.io.*;
import java.math.RoundingMode;
import java.util.concurrent.atomic.*;

/**
 * Mihail Dunaev, 331CC
 * Parallel and Distributed Algorithms
 * Assignment 2 - Indexing files with Map-Reduce
 */

public class Main {

	public static int nworkers, nswords, nrwords, file_fragment_size, nsdocs, nrdocs;
	public static String inputfilename, outputfilename;
	public static String[] words, docs;
	public static Scanner input;
	
	public static void main(String[] args) throws IOException {
		
		nworkers = Integer.parseInt(args[0]);
		inputfilename = args[1];
		outputfilename = args[2];
		
		// read input "input.txt"
		readInput();
		
		// needed workpools
		WorkpoolText wp_text = new WorkpoolText(); // for text fragments
		WorkpoolHash wp_hash = new WorkpoolHash(); // for word freq
		IndexedFilePacket[] indexed_files = new IndexedFilePacket[nsdocs];
		
		// read files, save fragments in workpool
		updateWorkpoolText(wp_text);
		
		// process fragments and save freq (workers)
		map_reduce(wp_text, wp_hash);
		
		// sort hashtable after freq
		sort(wp_hash, indexed_files);
		
		// look for first X words after freq
		search_and_write(indexed_files);
	}
	
	public static void search_and_write(IndexedFilePacket[] indexed_files) throws IOException{
		
		FileWriter fstream = new FileWriter(outputfilename);
		BufferedWriter out = new BufferedWriter(fstream);
		
		out.write(new String("Results for: ("));
		for (int i = 0; i < nswords; i++){
			if (i == (nswords-1)){
				out.write(words[i]);
				out.write(")\n\n");
			}
			else{
				out.write(words[i]);
				out.write(", ");
			}
		}

		// go through indexed files
		for (int i = 0; i < indexed_files.length; i++){
			int contains = 0;
			float[] freqs = new float[nswords];
			for (int j = 0; j < indexed_files[i].words.length; j++){
				for (int k = 0; k < nswords; k++){
					if (words[k].compareTo(indexed_files[i].words[j]) == 0){
						contains++;
						freqs[k] = indexed_files[i].freq[j];
					}
				}
			}
			
			// print, if we found all of them
			if (contains == nswords){
				out.write(indexed_files[i].filename);
				out.write(new String(" ("));
				
				DecimalFormat df = new DecimalFormat("##.##");
				df.setRoundingMode(RoundingMode.DOWN);

				for (int j = 0; j < nswords; j++){
					out.write(String.format("%.2f", Float.parseFloat(df.format(freqs[j]))));
					if (j == (nswords-1))
						out.write(")\n");
					else
						out.write(", ");
				}
			}
		}
		out.close();
	}
	
	public static void sort(WorkpoolHash wp_hash, IndexedFilePacket[] indexed_files){
		
		WorkerHash[] workers = new WorkerHash[nworkers];
		for (int i = 0; i < nworkers; i++){
			workers[i] = new WorkerHash(wp_hash, nrwords, indexed_files);
			workers[i].start();
		}
		for (int i = 0; i < nworkers; i++){			
			try {
				workers[i].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}		
	}
	
	public static void map_reduce(WorkpoolText wp_text, WorkpoolHash wp_hash){ 
	
		// packets with a hashtable for each file + info	
		HashPacket[] hashpackets = new HashPacket[nsdocs];
		for (int i = 0; i < nsdocs; i++)
			hashpackets[i] = new HashPacket(i, docs[i]);
		
		// our workers
		WorkerText[] workers = new WorkerText[nworkers];
		for (int i = 0; i < nworkers; i++){
			workers[i] = new WorkerText(wp_text, hashpackets, wp_hash);
			workers[i].start();
		}

		// wait for them to finish
		for (int i = 0; i < nworkers; i++){			
			try {
				workers[i].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}
	
	public static void updateWorkpoolText(WorkpoolText wp){
		
		for (int i = 0; i < docs.length; i++){
			try {
				input = new Scanner(new File(docs[i]));
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			}

			// all the file
			input.useDelimiter("\\Z");
			String file = input.next();
			// split after special characters, as requested
			String all_words[] = file.split("[ \n\\-().,?!:;\"#$%&\\*\\^'\\[\\]\\{\\}=\\+1234567890]+");
			
			int total_index = 0;
			
			while (total_index < all_words.length){
			
				int bytes_so_far = 0,
				words_so_far = 0;
				String words[] = new String[all_words.length];
				int index = 0;

				do {
					words[index] = all_words[total_index].toLowerCase();
					words_so_far++;
					bytes_so_far = bytes_so_far + words[index].length();
					index++;
					total_index++;
				} while ((bytes_so_far < file_fragment_size) && (total_index < all_words.length));

				boolean lastpacket = false;
				
				if (total_index >= all_words.length)
					lastpacket = true;
				
				TextPacket tp = new TextPacket(i, docs[i], words, words_so_far, lastpacket);
				wp.add(tp);
			}
			input.close();
		}
	}
	
	public static void readInput(){
			
		try {
			input = new Scanner(new File(inputfilename));
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}
		
		// NC
		nswords = input.nextInt();
		
		// searched words - not safe
		words = new String[nswords];
		for (int i = 0; i < nswords; i++)
			words[i] = input.next();
		
		// D, fragment sizes
		file_fragment_size = input.nextInt();
		
		// N
		nrwords = input.nextInt();
		
		// X, how many docs for results
		nrdocs = input.nextInt();

		// ND, in how many do we search
		nsdocs = input.nextInt();
		
		// the docs
		docs = new String[nsdocs];
		for (int i = 0; i < nsdocs; i++)
			docs[i] = input.next();

		// close input
		input.close();
	}

	public static void checkInput(){
		
		System.out.println(nswords);
		
		for (int i = 0; i < words.length; i++)
			System.out.print(words[i] + " ");
		System.out.println();
		
		System.out.println(file_fragment_size);
		System.out.println(nrwords);
		System.out.println(nrdocs);
		System.out.println(nsdocs);
		
		for (int i = 0; i < nsdocs; i++)
			System.out.println(docs[i]);
	}
}

class WorkpoolText {
	
	ConcurrentLinkedQueue<TextPacket> q;
	
	public WorkpoolText(){
		this.q = new ConcurrentLinkedQueue<TextPacket>();
	}
	
	public void add(TextPacket tp){
		q.add(tp);
	}
	
	public TextPacket take(){
		return q.poll();
	}
	
	public void printWP(){

		int size = q.size();
		
		if (size == 0)
			System.out.printf("Workpool with Text Packets is empty !\n\n");
		else{
			TextPacket[] tps = new TextPacket[size];
			q.toArray(tps);
			System.out.printf("Workpool with Text Packets --- Packets : %d ;\n\n", size);
			for (int i = 0; i < size; i++)
				System.out.printf("Packet %d : %s, words : %d\n", i, tps[i].filename, tps[i].nwords);
		}
	}
}

class TextPacket {
	
	public String filename;
	public int filenameid;
	public String[] words;
	public int nwords;
	boolean lastpacket;
	
	public TextPacket(int filenameid, String filename, String[] words, int nwords, boolean lastpacket){
		
		this.filenameid = filenameid;
		this.filename = filename;
		this.words = words;
		this.nwords = nwords;
		this.lastpacket = lastpacket;
	}
	
	public void printPacket(){
		
		System.out.printf("Filename : %s (id = %d) ; Words : %d\n", filename, filenameid, nwords);
		
		for (int i = 0; i < nwords; i++){
			System.out.print (words[i] + " ");
			if (((i+1) % 13) == 0)
				System.out.println();
		}
	}
	
	public void printPacketDetailed(){
		
		System.out.printf("Filename : %s (id = %d) ; Words : %d ; Lastpacket = %b\n", filename, filenameid, nwords, lastpacket);
		for (int i = 0; i < nwords; i++)
			System.out.printf("word[%d] = %s\n", i, words[i]);
	}
}

class WorkpoolHash {
	
	ConcurrentLinkedQueue<HashPacket> q;
	
	public WorkpoolHash(){
		this.q = new ConcurrentLinkedQueue<HashPacket>();
	}
	
	public void add(HashPacket hp){
		q.add(hp);
	}
	
	public HashPacket take(){
		return q.poll();
	}
	
	public void printWH(){
		
		int size = q.size();
		HashPacket[] hps = new HashPacket[size];
		q.toArray(hps);
		
		System.out.printf("Workpool with Hash Packets --- Packets : %d ;\n\n", size);
		
		for (int i = 0; i < size; i++)
			System.out.printf("Hashtable %d ; Filename : %s ; Words : %d ; Unique Words : %d\n", hps[i].id, hps[i].filename, hps[i].nwords.get(), hps[i].unique_words.get());
	}
}

class HashPacket {
	
	ConcurrentHashMap<String, AtomicInteger> hashmap;
	int id;
	AtomicInteger nwords, unique_words;
	String filename;
	
	public HashPacket(int id, String filename){
		
		this.id = id;
		this.hashmap = new ConcurrentHashMap<String, AtomicInteger>();
		this.nwords = new AtomicInteger(0);
		this.unique_words = new AtomicInteger(0);
		this.filename = filename;
	}
	
	public void printHashPacket(){
		
		System.out.printf("HashPacket %d ; Filename : %s ; Words : %d ; Unique Words : %d\n\n", id, filename, nwords.get(), unique_words.get());
		
		Enumeration<String> keys = hashmap.keys();
		Enumeration<AtomicInteger> freq = hashmap.elements();

		int value;
		String aux;
		
		while (keys.hasMoreElements()){
			
			aux = keys.nextElement();
			value = freq.nextElement().get();
			
			System.out.printf("%s ", aux);
			System.out.printf("%d\n", value);
		}
		System.out.println();
	}
}

class WorkerText extends Thread {
	
	WorkpoolText wp_text;
	WorkpoolHash wp_hash;
	boolean stop;
	HashPacket[] hashpackets;
	
	public WorkerText(WorkpoolText wp_text, HashPacket[] hashpackets, WorkpoolHash wp_hash){
		
		this.wp_text = wp_text;
		this.wp_hash = wp_hash;
		this.stop = false;
		this.hashpackets = hashpackets;
	}
	
	public void run(){
		while (stop == false){
			TextPacket tp = wp_text.take();
			if (tp == null)
				stop = true;
			else{
				int id = tp.filenameid;
				for (int i = 0; i < tp.nwords; i++){
					String word = tp.words[i];
					AtomicInteger aux = hashpackets[id].hashmap.putIfAbsent(word, new AtomicInteger(1));
					if (aux == null)
						hashpackets[id].unique_words.incrementAndGet();
					else
						hashpackets[id].hashmap.get(word).incrementAndGet();
					hashpackets[id].nwords.incrementAndGet();
				}
				// check max (packets) and if it's last one
				if (tp.lastpacket == true)
					wp_hash.add(hashpackets[id]);
			}
		}
	}
}

class WorkpoolIndexedFiles {
	
	ConcurrentLinkedQueue<IndexedFilePacket> q;
	public WorkpoolIndexedFiles(){
		this.q = new ConcurrentLinkedQueue<IndexedFilePacket>();
	}
	public void add(IndexedFilePacket ifp){
		q.add(ifp);
	}
	public IndexedFilePacket take(){
		return q.poll();
	}
	public void printWP(){
		int size = q.size();
		if (size == 0)
			System.out.printf("Workpool with Indexed File Packets is empty !\n\n");
		else{
			IndexedFilePacket[] ifps = new IndexedFilePacket[size];
			q.toArray(ifps);
			System.out.printf("Workpool with Indexed File Packets --- Packets : %d ;\n\n", size);
			for (int i = 0; i < size; i++)
				System.out.printf("File : %s ;  id : %d ; words : %d\n", ifps[i].filename, ifps[i].fileid, ifps[i].words.length );
		}
	}
}

class IndexedFilePacket {
	
	String filename;
	int fileid;
	String words[];
	float[] freq;
	
	public IndexedFilePacket(String filename, int fileid, WordFreqPair[] words, int nrwords, int nwords){
		
		this.fileid = fileid;
		this.filename = filename;
		
		// how many words we will take
		int size = nrwords;
		while ((size < words.length) && (words[size-1].freq == words[size].freq))
			size++;
		
		this.words = new String[size];
		this.freq = new float[size];
		for (int i = 0; i < size; i++){
			this.words[i] = words[i].word;
			this.freq[i] = (float)(words[i].freq * 100)/nwords;
		}
	}
	
	public void printIndexedFP(){
		System.out.printf("Indexed File : %s ; id = %d ;\n\n", filename, fileid);
		for (int i = 0; i < words.length; i++)
			System.out.printf("%s %f \n", words[i], freq[i]);
		System.out.println();
	}
}

class WorkerHash extends Thread {
	
	WorkpoolHash wp_hash;
	IndexedFilePacket[] indexed_files;
	int nrwords;
	boolean stop;
	
	public WorkerHash(WorkpoolHash wp_hash, int nrwords, IndexedFilePacket[] indexed_files){
		this.wp_hash = wp_hash;
		this.indexed_files = indexed_files;
		this.stop = false;
		this.nrwords = nrwords;
	}
	public void run(){
		while (stop == false){
			HashPacket hp = wp_hash.take();
			if (hp == null)
				stop = true;
			else{
				Enumeration<String> keys = hp.hashmap.keys();
				Enumeration<AtomicInteger> freq = hp.hashmap.elements();
				WordFreqPair[] aux = new WordFreqPair[hp.unique_words.get()];
				int index = 0;
				
				while (keys.hasMoreElements()){	
					aux[index] = new WordFreqPair(keys.nextElement(), freq.nextElement().get());
					index++;
				}
				
				Arrays.sort(aux);
				IndexedFilePacket ifp = new IndexedFilePacket(hp.filename, hp.id, aux, nrwords, hp.nwords.get());
				indexed_files[hp.id] = ifp;
			}
		}
	}
}

class WordFreqPair implements Comparable<WordFreqPair>{
	
	String word;
	int freq;

	public WordFreqPair(String word, int freq){		
		this.word = word;
		this.freq = freq;
	}

	@Override
	public int compareTo(WordFreqPair arg0) {
		
		// sort descending
		if (freq > arg0.freq)
			return -1;
		if (freq == arg0.freq)
			return 0;

		return 1; // freq < aux.freq	
	}
}
