## IP Address caching
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; I used a hashtable to implement caching because I wanted to review the data structure for technical interviews and the entry/lookup time is O(1)

## Page caching
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; I used the timestamp of the files to implement caching. One potential issues is that I don't have a mutex around the creating of the file but that shouldn't be an issue because each thread handles a request. Page caching doesn't work for gzip encoding and the page may not load all the way if a page is sent from the cache that is gzip encoded. 

## Compile and Run
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; make compile; make run
