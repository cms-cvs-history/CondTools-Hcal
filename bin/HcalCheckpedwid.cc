#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <map>
#include <vector>


using namespace std;

int main (int argc , char** argv)
{
  if (argc<4)
    {
      cout << "Syntax: checkpeds newpeds.txt oldpeds.txt resultpeds.txt" << endl;
      return 1;
    }

  FILE* DATA1;
  //  char FILEDATA1[120] = "pedsSteve/output/29930-peds_fC.txt";
  char * FILEDATA1 = argv[1];
  DATA1 = fopen(FILEDATA1,"r");
  int file1_status=0;
  
  FILE* DATA2;
  //  char FILEDATA2[120] = "gren_development_ped_leveling.txt";
  char * FILEDATA2 = argv[2];
  DATA2 = fopen(FILEDATA2,"r");
  int file2_status=0;
  
  FILE* DATA3;
  //  char FILEDATA3[120] = "outpeds.txt";
  char * FILEDATA3 = argv[3];   
  DATA3 = fopen(FILEDATA3,"w");
  int file3_status=0;

  int a1, a2, a3;
  float f1, f2, f3, f4, f5, f6, f7, f8, f9, f10;
  long id;

  char* subdet;
  //char subdet[10];
  int intsubdet = 0;
  int line1 = 0;

  typedef map <long,int> mymaptype;
  typedef mymaptype::value_type mymapvalue;

  mymaptype mymap;
  vector<vector<int> > myas;
  vector<vector<float> > myfs;
  vector<char*> mysubdet;
  //vector<long> myDetIds;
  typedef vector<int> vecint;

  do
    {
      line1++;
//      if (line == 1)
//	{
//	  file1_status = fscanf(DATA1," \n",&a1, &a2, &a3,
//				&subdet, &f1, &f2, &f3, &f4, &id);
//	}
      a1=0; a2=0; a3=0; f1=0; f2=0; f3=0; f4=0; f5=0; f6=0; f7=0; f8=0; f9=0; f10=0; id=0;
      file1_status = fscanf(DATA1," %d %d %d %s %f %f %f %f %f %f %f %f %f %f %X \n",&a1, &a2, &a3,
			    &subdet, &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9, &f10, &id);
      if (file1_status !=EOF)
	{
	  //	  printf("readout: %d %d %d %s %f %f %f %f %X %d\n",a1, a2, a3, &subdet, f1, f2, f3, f4, id, id);
	  vector<int> aline;
	  aline.push_back(a1);
	  aline.push_back(a2);
	  aline.push_back(a3);
	  myas.push_back(aline);
	  vector<float> fline;
	  fline.push_back(f1);
	  fline.push_back(f2);
	  fline.push_back(f3);
	  fline.push_back(f4);
	  fline.push_back(f5);
	  fline.push_back(f6);
	  fline.push_back(f7);
	  fline.push_back(f8);
	  fline.push_back(f9);
	  fline.push_back(f10);
	  myfs.push_back(fline);
	  mysubdet.push_back(subdet);
	  //	  myDetIds.push_back(id);
	  mymap.insert(mymapvalue(id,(line1-1)));
        }
    }
  while(file1_status != EOF ); // loop over events in one data file

  std::cout << "file 1 read  "; // << endl << endl;
  std::cout << mymap.size() << " lines"  << std::endl;

  int line2 = 0;
  do
    {
      line2++;
      a1=0; a2=0; a3=0; f1=0; f2=0; f3=0; f4=0; id=0;
      file2_status = fscanf(DATA2," %d %d %d %s %f %f %f %f %f %f %f %f %f %f %10X \n",&a1, &a2, &a3, &subdet, &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9, &f10, &id);

      if (file2_status !=EOF)
	{
	  vector<int> bline;
	  bline.push_back(a1);
	  bline.push_back(a2);
	  bline.push_back(a3);

	  if (mymap.find(id) == mymap.end())
	    {
	      std::cout << "s"; //>>> eta,phi,depth not found !  ";
	      //	      printf("readout: %d %d %d %s %f %f %f %f %X \n",a1, a2, a3, &subdet, f1, f2, f3, f4, id);
	      fprintf (DATA3, "  %15d %15d %15d %15s %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %10X\n", a1, a2, a3, &subdet, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, id);
	    }
	  else
	    {
	      int myind = mymap.find(id)->second;
	      vector<int> mya = myas.at(myind);
	      if (bline != mya) std::cout << "*ATTN: for detid different indices "<< id << std::endl;

	      vector<float> myf = myfs.at(myind);
	      char* mysub = mysubdet.at(myind);
	      long myid = mymap.find(id)->first;

	      std::cout << "f";
	      fprintf (DATA3, "  %15d %15d %15d %15s %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %10X\n", 
		       mya.at(0), mya.at(1), mya.at(2), &mysub,
		       myf.at(0), myf.at(1), myf.at(2), myf.at(3), myf.at(4), myf.at(5), myf.at(6), myf.at(7), myf.at(8), myf.at(9), myid);

	      mymap.erase(id);
	    }
	}
    }
  while(file2_status != EOF ); // loop over events in one data file
  
  std::cout << std::endl << "leftover from file1:" << std::endl;
  for (mymaptype::iterator mit = mymap.begin(); mit != mymap.end(); mit++)
    {
      int myind = mit->second;
      vector<float> myf = myfs.at(myind);
      vector<int> mya = myas.at(myind);
      char* mysub = mysubdet.at(myind);
      long myid = mit->first;
      
      fprintf (DATA3, "  %15d %15d %15d %15s %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %10X\n", 
	       mya.at(0), mya.at(1), mya.at(2), &mysub,
	       myf.at(0), myf.at(1), myf.at(2), myf.at(3), myf.at(4), myf.at(5), myf.at(6), myf.at(7), myf.at(8), myf.at(9), myid);
      
    }
  
  std::cout << std::endl;
  std::cout << "file 1: " << line1 << " file 2: " << line2 << std::endl;
  
  
  
  std::cout << "finished..." << std::endl;

}
