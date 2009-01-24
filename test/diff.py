#!/usr/bin/env python
import os

afs_loc="/afs/cern.ch/cms/HCAL/document/904/survival/mapping/Jared/03-sep-2008/"
date1_string="_Sep.03.2008.txt"
date2_string="_Jan.23.2009.txt"

maps=["HCALmapHBEF","HCALmapHT","HCALmapCALIB","HCALmapHO","ZDCmap"]

for map in maps:
    ref=afs_loc+map+date1_string
    if (map=="HCALmapHT") : ref=afs_loc+"HCALmapHTrigger"+date1_string
    new=map+date2_string
    cmd="diff "+ref+" "+new
    print cmd
    os.system(cmd)
