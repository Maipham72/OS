#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int pageNo;
  int modified;
  // int dirty;
  int referenced;
  unsigned long lastAccessTime;
} page;

enum repl { randoms, fifo, lru, clock };
int createMMU(int);
int checkInMemory(int);
int allocateFrame(int);
page selectVictim(int, enum repl);
const int pageoffset = 12; /* Page size is fixed to 4 KB */
int numFrames;

// variable to keep track of which index the hand is
// pointing to at the moment
int clockHand = 0;

page *framesArray = NULL;
int totalFrames;

/* Creates the page table structure to record memory allocation */
int createMMU(int frames) {
  numFrames = frames;

  framesArray = (page *)malloc(sizeof(page) * numFrames);
  if (framesArray == NULL) {
    printf("Unable to allocate memory");
    return -1;
  }

  for (int i = 0; i < numFrames; i++) {
    framesArray[i].pageNo = -1;
    // framesArray[i].dirty = 0;
    framesArray[i].referenced = 0;
    framesArray[i].lastAccessTime = 0;
  }

  return 0;
}

void freeMMU() {
  if (framesArray != NULL) {
    free(framesArray);
    framesArray = NULL;
  }
}

/* Checks for residency: returns frame no or -1 if not found */
int checkInMemory(int page_number) {
  int result = -1;
  // to do
  for (int i = 0; i < numFrames; i++) {
    if (framesArray[i].pageNo == page_number) {
      result = i;
      return result;
    };
  }

  return result;
}

/* allocate page to the next free frame and record where it put it */
int allocateFrame(int page_number) {
  // to do
  for (int i = 0; i < numFrames; i++) {
    if (framesArray[i].pageNo == -1) {
      framesArray[i].pageNo = page_number;
      framesArray[i].modified = 0;
      framesArray[i].referenced = 1;

      return i;
    }
  }
  return -1;
}
/* Selects a victim for eviction/discard according to the replacement
 * algorithm,  returns chosen frame_no  */
page selectVictim(int page_number, enum repl mode) {
  page victim;
  // to do
  victim.pageNo = 0;
  victim.modified = 0;

  if (mode == 0) {
    // rand
    int number = rand() % numFrames;

    victim.pageNo = framesArray[number].pageNo;
  } else if (mode == 1) {
    // fifo

  } else if (mode == 2) {
    // lru

  } else if (mode == 3) {
    // clock

    // check the first page. continue until a referenced bit = 0 is found

    // acts as a boolean variable equivalent
    // done = 1 signifies that the referenced bit = 0 has been found
    int done = 0;
    while (done != 1) {
      if (framesArray[clockHand].referenced == 1) {
        // reset to 0
        framesArray[clockHand].referenced = 0;

        // increment to next
        clockHand++;

        if (clockHand >= numFrames) {
          clockHand = 0;  // reset to beginning
        }

      } else {
        victim = framesArray[clockHand];
        framesArray[clockHand].pageNo = page_number;
        framesArray[clockHand].modified = 0;
        framesArray[clockHand].referenced = 1;

        // increment to next
        clockHand++;

        if (clockHand >= numFrames) {
          clockHand = 0;  // reset to beginning
        }

        // stop while loop
        done = 1;
      }
    }
  }
  return (victim);
}

void setReferenced(int page_number) {
  for (int i = 0; i < numFrames; i++) {
    if (framesArray[i].pageNo == page_number) {
      // if page number is present in array, set its
      // referenced bit to 1 and return.
      framesArray[i].referenced = 1;
      return;
    }
  }
}

int main(int argc, char *argv[]) {
  char *tracename;
  int page_number, frame_no, done;
  int do_line, i;
  int no_events, disk_writes, disk_reads;
  int debugmode;
  enum repl replace;
  int allocated = 0;
  int victim_page;
  unsigned address;
  char rw;
  page Pvictim;
  FILE *trace;

  if (argc < 5) {
    printf(
        "Usage: ./memsim inputfile numberframes replacementmode debugmode "
        "\n");
    exit(-1);
  } else {
    tracename = argv[1];
    trace = fopen(tracename, "r");
    if (trace == NULL) {
      printf("Cannot open trace file %s \n", tracename);
      exit(-1);
    }
    numFrames = atoi(argv[2]);
    if (numFrames < 1) {
      printf("Frame number must be at least 1\n");
      exit(-1);
    }

    // allocate memory to store the framesArray
    framesArray = malloc(sizeof(page) * numFrames);

    if (strcmp(argv[3], "lru\0") == 0)
      replace = lru;
    else if (strcmp(argv[3], "rand\0") == 0)
      replace = randoms;
    else if (strcmp(argv[3], "clock\0") == 0)
      replace = clock;
    else if (strcmp(argv[3], "fifo\0") == 0)
      replace = fifo;
    else {
      printf("Replacement algorithm must be rand/fifo/lru/clock  \n");
      exit(-1);
    }

    if (strcmp(argv[4], "quiet\0") == 0)
      debugmode = 0;
    else if (strcmp(argv[4], "debug\0") == 0)
      debugmode = 1;
    else {
      printf("Replacement algorithm must be quiet/debug  \n");
      exit(-1);
    }
  }

  done = createMMU(numFrames);
  if (done == -1) {
    printf("Cannot create MMU");
    exit(-1);
  }
  no_events = 0;
  disk_writes = 0;
  disk_reads = 0;

  do_line = fscanf(trace, "%x %c", &address, &rw);
  while (do_line == 2) {
    page_number = address >> pageoffset;
    frame_no = checkInMemory(page_number); /* ask for physical address */

    if (frame_no == -1) {
      disk_reads++; /* Page fault, need to load it into memory */
      if (debugmode) printf("Page fault %8d \n", page_number);
      if (allocated < numFrames) /* allocate it to an empty frame */
      {
        frame_no = allocateFrame(page_number);
        allocated++;
      } else {
        Pvictim = selectVictim(page_number,
                               replace); /* returns page number of the victim */
        frame_no = checkInMemory(
            page_number);     /* find out the frame the new page is in */
        if (Pvictim.modified) /* need to know victim page and modified  */
        {
          disk_writes++;
          if (debugmode) printf("Disk write %8d \n", Pvictim.pageNo);
        } else if (debugmode) {
          printf("Discard    %8d \n", Pvictim.pageNo);
        }
      }
    } else {
      // if page in array and not initial allocation, set its referenced bit to
      // 1
      setReferenced(page_number);
    }

    if (rw == 'R') {
      if (debugmode) printf("reading    %8d \n", page_number);
    } else if (rw == 'W') {
      framesArray[frame_no].modified = 1;

      // mark page in page table as written - modified
      if (debugmode) printf("writting   %8d \n", page_number);
    } else {
      printf("Badly formatted file. Error on line %d\n", no_events + 1);
      exit(-1);
    }

    no_events++;
    do_line = fscanf(trace, "%x %c", &address, &rw);
  }

  printf("total memory frames:  %d\n", numFrames);
  printf("events in trace:      %d\n", no_events);
  printf("total disk reads:     %d\n", disk_reads);
  printf("total disk writes:    %d\n", disk_writes);
  printf("page fault rate:      %.4f\n", (float)disk_reads / no_events);
}
