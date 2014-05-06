#include<stdio.h>
#include<stdlib.h>
#include<string.h>

// Global declarations
int block_bits = 2, tag_mask = 0, index_mask = 0, bm_pc_bits=0, index_bits=0, lrucount=0, sets=0;
int evict_block=0;

// Variables for storing the command line arguments
int chooser_pc_bits=0;
int pc_bits=0; // Number of PC bits
int ghr_bits=0;
int btb_size=0; 
int btb_assoc=0; 
char trace_file[20]; // input trace file

// Structure for the BTB cache and each tag
typedef struct {
   int tag;
   unsigned valid;
   unsigned LRU_counter;
} tag;

// Function prototype declarations
void bimodal(void);
void gshare(void);
void hybrid(void);

// Function to calculate the power of a value
int power(int x, unsigned int y)
{
    int temp=0;
    if( y == 0)
        return 1;
    temp = power(x, y/2);
    if (y%2 == 0)
        return temp*temp;
    else
        return x*temp*temp;
}

tag** create_cache(unsigned rows, int columns) {
 
   int i = 0;
   tag **a;  
   a = calloc(rows, sizeof(tag*));
   for(i = 0; i < rows; i++)
      a[i] = calloc(columns, sizeof(tag));   
   return a; 
}

   tag** tag1 = NULL;

// Function to create a mask from 'a' bit position to 'b' bit position. 
// http://stackoverflow.com/questions/8011700/how-do-i-extract-specific-n-bits-of-a-32-bit-unsigned-integer-in-c
unsigned createMask(unsigned a, unsigned b) {

   unsigned i, r = 0;
   for (i = a; i < b; i++)
       r |= 1 << i;
   return r;
}


//Based on the replacement policy return the appropriate block for the index_value
int evict(int assoc, int index_value ) {
   unsigned j=0, selected_block=0, temp=0;
   selected_block = j;

      temp = tag1[index_value][0].LRU_counter;

      // To find the block with minimum LRU_counter
      for(j=0; j< assoc; j++) {
	   if (tag1[index_value][j].LRU_counter < temp) {
		   temp = tag1[index_value][j].LRU_counter;
		   selected_block = j;
	   }
      }
   return selected_block;
}


void set_btb() {

      unsigned int temp=0;

   // Calculate number of sets 
      sets = btb_size / (4 * btb_assoc);

   // Create a tag matrix before reading from trace file

      tag1 = create_cache(sets,btb_assoc);

   // Create BTB mask according to number of sets
      temp = sets;
      while (temp >>= 1)
	   index_bits++;

      index_mask = createMask(0,index_bits);
      tag_mask = createMask(index_bits,32-block_bits);
      tag_mask >>= index_bits;

}

// Main function of the program
int main(int argc, char **argv) {


// Parse the command line arguments according to the branch predictor configuration
   if (strcmp(argv[1],"bimodal") == 0) {
 
      pc_bits		 = atoi(argv[2]); 
      btb_size		 = atoi(argv[3]);
      btb_assoc		 = atoi(argv[4]);
      strcpy(trace_file, argv[5]);
      
      if ( btb_size != 0)
  	 set_btb();

// Call the bimodal function to process the input file only if BTB hit!!
      bimodal();

   }

   else if (strcmp(argv[1],"gshare") == 0) {
 
      pc_bits		 = atoi(argv[2]); 
      ghr_bits 		 = atoi(argv[3]);
      btb_size		 = atoi(argv[4]);
      btb_assoc		 = atoi(argv[5]);
      strcpy(trace_file, argv[6]);

      if ( btb_size != 0)
  	 set_btb();

// Call the gshare function to process the input file only if BTB hit!!
      gshare();

   }

   else if (strcmp(argv[1],"hybrid") == 0) {
 
      chooser_pc_bits	 = atoi(argv[2]);
      pc_bits		 = atoi(argv[3]); 
      ghr_bits		 = atoi(argv[4]);
      bm_pc_bits	 = atoi(argv[5]); 
      btb_size		 = atoi(argv[6]);
      btb_assoc		 = atoi(argv[7]);
      strcpy(trace_file, argv[8]);

      if ( btb_size != 0)
  	 set_btb();

// Call the hybrid function to process the input file
      hybrid();

   }

   else {

      printf("\n Please enter the correct input");
      exit(1);
   }

   return 0;
}



void bimodal() {

// Local variable declaration starts here
   
   char line[100];
   char a[10];
   char outcome;
   int pc=0, i_mask=0;
   int full_addr = 0, addr = 0, bm_index = 0;
   unsigned rows = 0, i=0;
   float miss_rate=0;
   long int predictions=0, mispredictions=0;
   FILE *fp;

// BTB additions
   int index_value=0, branch_predictions=0;
   int tag_value=0, j=0, found=0, m=0, n=0;
   long btb_mispredictions=0;



// Creating the prediction table for bimodal prediction
   int *prediction_table = NULL;
   rows = power(2, pc_bits);
   prediction_table = calloc(rows, sizeof(int));

   for (i=0; i<rows; i++)
	prediction_table[i] = 2;

// Create the mask according to the number of pc_bits
   i_mask = createMask(0,pc_bits);
   //i_mask >>= block_bits;
	
// Read the input trace file and store the command and address
   fp = fopen(trace_file,"r");
   while (fgets(line,100,fp) != 0) {
           found = 0;
	   predictions++;
	   lrucount++;
	   sscanf(line,"%x %c", &pc, &outcome); 
	      	//full_addr = (int) strtol(a,NULL,16);
  		//addr = pc >> block_bits;

		bm_index = (pc >> block_bits) & i_mask;

//************************************************
// BTB additions   
		index_value = (pc >> block_bits) & index_mask;
   		tag_value = ((pc >> block_bits) >> index_bits) & tag_mask;

   if (btb_size != 0) {

   // Scan all blocks in that row of index_value and update counters if found
      for(j=0;j < btb_assoc; j++) {
	   if (tag1[index_value][j].tag == tag_value) {
		   if (tag1[index_value][j].valid == 1) {  // valid 1 means a non-empty block
		      branch_predictions++;
		      tag1[index_value][j].LRU_counter = lrucount;
		      found=1;

			if (outcome == 't') {
				if (prediction_table[bm_index] < 2)
					mispredictions++;

				if (prediction_table[bm_index] != 3)
					prediction_table[bm_index]++;
			}
			else {
				if (prediction_table[bm_index] >= 2)
					mispredictions++;

				if (prediction_table[bm_index] != 0)
					prediction_table[bm_index]--;
			}

		      break;
		   }
	   }
      }
     


// If it is a miss and the input for BTB is non-zero
     if (0 == found) {
 	if (outcome == 't')
		btb_mispredictions++;

   	for(j=0; j < btb_assoc;j++) {
	      if (tag1[index_value][j].valid == 0) {
		   tag1[index_value][j].tag = tag_value;
		   tag1[index_value][j].LRU_counter = lrucount;
	   	   tag1[index_value][j].valid = 1;
		   break;
	      }	


	if ((j == btb_assoc-1) && (tag1[index_value][j].valid == 1)) {

	   evict_block = evict(btb_assoc, index_value);
	   tag1[index_value][evict_block].tag = tag_value;
	   tag1[index_value][evict_block].LRU_counter = lrucount;
	   tag1[index_value][evict_block].valid = 1;
 
	}
        }
    }
}
	
   else {

//********************************************	
// Check the actual outcome of the branch and update the prediction rable row accordingly
		if (outcome == 't') {
			if (prediction_table[bm_index] < 2)
				mispredictions++;

			if (prediction_table[bm_index] != 3)
				prediction_table[bm_index]++;
		}
		else {
			if (prediction_table[bm_index] >= 2)
				mispredictions++;

			if (prediction_table[bm_index] != 0)
				prediction_table[bm_index]--;
		}
	

   	   }
   }

   fclose(fp);

   printf(" COMMAND");
   printf("\n ./sim %s %d %d %d %s", "bimodal", pc_bits, btb_size, btb_assoc, trace_file);
   printf("\n OUTPUT");

   if (btb_size != 0) {
      miss_rate = (float) (btb_mispredictions + mispredictions) / predictions;

      printf("\n size of BTB:  %d", btb_size);
      printf("\n number of branches: %ld", predictions );
      printf ("\n number of predictions from branch predictor: %d" ,branch_predictions);
      printf ("\n number of mispredictions from branch predictor: %ld " , mispredictions );
      printf("\n number of branches miss in BTB and taken: %ld", btb_mispredictions);
      printf("\n total mispredictions: %ld", btb_mispredictions + mispredictions);
      printf("\n misprediction rate: %.2f%%\n", miss_rate*100);

      printf("\n FINAL BTB CONTENTS");
      for (m=0; m < sets; m++) {
         printf("\n set %d :\t",m);
	 for (n=0; n < btb_assoc; n++) {
		printf(" %x ",tag1[m][n].tag);

	}
      }
      printf("\n");	
   }

   else {

      miss_rate  = (float)(mispredictions * 100)/predictions;


      printf("\n number of predictions: %ld", predictions);
      printf("\n number of mispredictions: %ld", mispredictions);
      printf("\n misprediction rate: %.2f%%", miss_rate);
   }

      printf("\n FINAL BIMODAL CONTENTS\n");
      for (i=0; i<rows; i++) 
         printf(" %d\t%d\n",i,prediction_table[i]);

}

void gshare() {

// Local variable declaration starts here
   
   char line[100];
   char a[10];
   char outcome;
   int pc=0, i_mask=0;
   int full_addr = 0, addr = 0, bm_index = 0, ghr_index=0, actual_outcome=0;
   unsigned rows = 0, i=0; 
   float miss_rate=0;
   long int predictions=0, mispredictions=0;
   FILE *fp;

   long int ghr=0, ghr_max=0;
   ghr_max = createMask(0,ghr_bits);
   ghr=0;

// BTB additions
   int index_value=0, branch_predictions=0;
   int tag_value=0, j=0, found=0, m=0, n=0;
   long btb_mispredictions=0;

  
// Creating the prediction table for gshare prediction
   int *prediction_table = NULL;
   rows = power(2, pc_bits);
   prediction_table = calloc(rows, sizeof(int));

   for (i=0; i<rows; i++)
	prediction_table[i] = 2;

// Create the mask according to the number of pc_bits
   i_mask = createMask(0,pc_bits);
   //i_mask >>= block_bits;
	

// Read the input trace file and store the command and address
   fp = fopen(trace_file,"r");
   while (fgets(line,100,fp) != 0) {
	   found=0;
	   lrucount++;
	   predictions++;
	   sscanf(line,"%x %c", &pc, &outcome); 
      	//full_addr = (int) strtol(a,NULL,16);
 	//addr = pc >> block_bits;
	bm_index = (pc >> block_bits) & i_mask;
	ghr_index = (ghr << (pc_bits - ghr_bits)) ^ bm_index; // This is the ghr index for prediction table

//************************************************
// BTB additions   
                index_value = (pc >> block_bits) & index_mask;
                tag_value = ((pc >> block_bits) >> index_bits) & tag_mask;

   if (btb_size != 0) {

   // Scan all blocks in that row of index_value and update counters if found
      for(j=0;j < btb_assoc; j++) {
           if (tag1[index_value][j].tag == tag_value) {
                   if (tag1[index_value][j].valid == 1) {  // valid 1 means a non-empty block
                      branch_predictions++;
                      tag1[index_value][j].LRU_counter = lrucount;
                      found=1;

                        if (outcome == 't') {
			        actual_outcome = 1;
                                if (prediction_table[ghr_index] < 2)
                                        mispredictions++;

                                if (prediction_table[ghr_index] != 3)
                                        prediction_table[ghr_index]++;
                        }
                        else {
			        actual_outcome = 0;
                                if (prediction_table[ghr_index] >= 2)
                                        mispredictions++;

                                if (prediction_table[ghr_index] != 0)
                                        prediction_table[ghr_index]--;
                        }
// Updating the global branch history register
			ghr = (ghr >> 1) | (actual_outcome << (ghr_bits - 1));

                      break;
                   }
           }
      }


// If it is a miss and the input for BTB is non-zero
     if (0 == found) {
        if (outcome == 't')
                btb_mispredictions++;

        for(j=0; j < btb_assoc;j++) {
              if (tag1[index_value][j].valid == 0) {
                   tag1[index_value][j].tag = tag_value;
                   tag1[index_value][j].LRU_counter = lrucount;
                   tag1[index_value][j].valid = 1;
                   break;
              }


        if ((j == btb_assoc-1) && (tag1[index_value][j].valid == 1)) {

           evict_block = evict(btb_assoc, index_value);
           tag1[index_value][evict_block].tag = tag_value;
           tag1[index_value][evict_block].LRU_counter = lrucount;
           tag1[index_value][evict_block].valid = 1;

        }
        }
    }
}

   else {
	
// Check the actual outcome of the branch and update the prediction rable row accordingly
		if (outcome == 't') {
			actual_outcome = 1;
			if (prediction_table[ghr_index] < 2)
				mispredictions++;

			if (prediction_table[ghr_index] != 3)
				prediction_table[ghr_index]++;
		}
		else {
			actual_outcome = 0;
			if (prediction_table[ghr_index] >= 2)
				mispredictions++;

			if (prediction_table[ghr_index] != 0)
				prediction_table[ghr_index]--;
		}
// Updating the global branch history register
		ghr = (ghr >> 1) | (actual_outcome << (ghr_bits - 1));

   }

   }
   fclose(fp);

   printf(" COMMAND");
   printf("\n ./sim %s %d %d %d %d %s", "gshare", pc_bits, ghr_bits, btb_size, btb_assoc, trace_file);
   printf("\n OUTPUT");

   if (btb_size != 0) {
      miss_rate = (float) (btb_mispredictions + mispredictions) / predictions;

      printf("\n size of BTB:  %d", btb_size);
      printf("\n number of branches: %ld", predictions );
      printf ("\n number of predictions from branch predictor: %d" ,branch_predictions);
      printf ("\n number of mispredictions from branch predictor: %ld " , mispredictions );
      printf("\n number of branches miss in BTB and taken: %ld", btb_mispredictions);
      printf("\n total mispredictions: %ld", btb_mispredictions + mispredictions);
      printf("\n misprediction rate: %.2f%%\n", miss_rate*100);

      printf("\n FINAL BTB CONTENTS");
      for (m=0; m < sets; m++) {
         printf("\n set %d :\t",m);
         for (n=0; n < btb_assoc; n++) {
                printf(" %x ",tag1[m][n].tag);

        }
      }
      printf("\n");
   }

   else {

      miss_rate  = (float)(mispredictions * 100)/predictions;

      printf("\n number of predictions: %ld", predictions);
      printf("\n number of mispredictions: %ld", mispredictions);
      printf("\n misprediction rate: %.2f%%", miss_rate);
   }

      printf("\n FINAL GSHARE CONTENTS\n");
      for (i=0; i<rows; i++)
         printf(" %d\t%d\n",i,prediction_table[i]);
}


void hybrid() {

// Local variable declaration starts here
   
   char line[100];
   char a[10];
   char outcome;
   int pc=0, chooser_mask=0, gshare_mask=0, i_mask=0;
   int full_addr = 0, addr = 0, chooser_index=0, bm_index = 0, ghr_index=0, actual_outcome=0, p_index=0;
   unsigned rows_bimodal = 0, rows_gshare = 0, rows_chooser = 0, i=0; 
   float miss_rate=0;
   long int predictions=0, mispredictions=0;
   FILE *fp;

   long int ghr=0;

// Creating the prediction table for gshare prediction
   int *g_prediction_table = NULL;
   rows_gshare = power(2, pc_bits);
   g_prediction_table = calloc(rows_gshare, sizeof(int));

   for (i=0; i<rows_gshare; i++)
	g_prediction_table[i] = 2;

// Creating the prediction table for bimodal prediction
   int *b_prediction_table = NULL;
   rows_bimodal = power(2, bm_pc_bits);
   b_prediction_table = calloc(rows_bimodal, sizeof(int));

   for (i=0; i<rows_bimodal; i++)
	b_prediction_table[i] = 2;

// Creating the chooser table for hybrid prediction
   int *chooser_table = NULL;
   rows_chooser = power(2, chooser_pc_bits);
   chooser_table = calloc(rows_chooser, sizeof(int));

   for (i=0; i<rows_chooser; i++)
	chooser_table[i] = 1;

// Create the mask according to the number of pc_bits for gshare table
   i_mask = createMask(0,bm_pc_bits);

// Create the mask according to the number of pc_bits for gshare table
   gshare_mask = createMask(0,pc_bits);

// Create the chooser mask according to the number of pc_bits
   chooser_mask = createMask(0,chooser_pc_bits);

// Read the input trace file and store the command and address
   fp = fopen(trace_file,"r");
   while (fgets(line,100,fp) != 0) {
	   predictions++;
	   p_index = 0;
	   if (sscanf(line,"%x %c", &pc, &outcome) != 0) {
	      	//full_addr = (int) strtol(a,NULL,16);
  		//addr = pc >> block_bits;

// Determine prediction_index for bimodal/gshare and chooser_index for hybrid
		bm_index = (pc >> block_bits) & i_mask;
		ghr_index = (ghr << (pc_bits - ghr_bits)) ^ ((pc >> block_bits) & gshare_mask);
		chooser_index = (pc >> block_bits) & chooser_mask;

// Updating the counter values in the chooser table
	       if (outcome == 't') {
		       actual_outcome = 1;
		       if ( g_prediction_table[ghr_index] >=2 && b_prediction_table[bm_index] < 2) {
			       if (chooser_table[chooser_index] < 3)
				       p_index = 1;
		       }
		       else if ( g_prediction_table[ghr_index] < 2 && b_prediction_table[bm_index] >= 2) {
			       if (chooser_table[chooser_index] > 0)
				       p_index = 2;
		       }

		       if (chooser_table[chooser_index] > 1) {
			       
			       if (g_prediction_table[ghr_index] < 2)
				       mispredictions++;
			       if (g_prediction_table[ghr_index] < 3)
				       g_prediction_table[ghr_index]++;
		       }

		       else {
			       
			       if (b_prediction_table[bm_index] < 2)
				       mispredictions++;
			       if (b_prediction_table[bm_index] < 3)
				       b_prediction_table[bm_index]++;
		       }
	       }

	       else if (outcome == 'n') {
		       actual_outcome = 0;
		       if ( g_prediction_table[ghr_index] >=2 && b_prediction_table[bm_index] < 2) {
			       if (chooser_table[chooser_index] > 0)
				       p_index = 2;
		       }
		       else if ( g_prediction_table[ghr_index] < 2 && b_prediction_table[bm_index] >= 2) {
			       if (chooser_table[chooser_index] < 3)
				       p_index = 1;
		       }

		       if (chooser_table[chooser_index] > 1) {
			       
			       if (g_prediction_table[ghr_index] > 1)
				       mispredictions++;
			       if (g_prediction_table[ghr_index] > 0)
				       g_prediction_table[ghr_index]--;
		       }

		       else {
			       
			       if (b_prediction_table[bm_index] > 1)
				       mispredictions++;
			       if (b_prediction_table[bm_index] > 0)
				       b_prediction_table[bm_index]--;
		       }
		       
	       }

// Updating the chooser_table contents

		if (1 == p_index)
			chooser_table[chooser_index]++;
		else if (2 == p_index)
			chooser_table[chooser_index]--;

// Updating the global branch history register
		ghr = (ghr >> 1) | (actual_outcome << (ghr_bits - 1));

   	   }
   }
   fclose(fp);

   miss_rate  = (float)(mispredictions * 100)/predictions;

   // Printing the output for hybrid

   printf(" COMMAND");
   printf("\n ./sim %s %d %d %d %d %d %d %s", "hybrid", chooser_pc_bits, pc_bits, ghr_bits, bm_pc_bits, btb_size, btb_assoc, trace_file);
   printf("\n OUTPUT");

   printf("\n number of predictions: %ld", predictions);
   printf("\n number of mispredictions: %ld", mispredictions);
   printf("\n misprediction rate: %.2f%%", miss_rate);

   printf("\n FINAL CHOOSER CONTENTS\n");
   for (i=0; i<rows_chooser; i++) 
      printf(" %d\t%d\n",i,chooser_table[i]);

   printf(" FINAL GSHARE CONTENTS\n");
   for (i=0; i<rows_gshare; i++) 
      printf(" %d\t%d\n",i,g_prediction_table[i]);

   printf(" FINAL BIMODAL CONTENTS\n");
   for (i=0; i<rows_bimodal; i++) 
      printf(" %d\t%d\n",i,b_prediction_table[i]);
}
