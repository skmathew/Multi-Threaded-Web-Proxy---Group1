# Multi-Threaded-Web-Proxy---Group1

Included files:

Documentation:
README.md
Report_Group1.docx
MathewSK Ind-Jour.docx


Source code:
ProxyServer.c
blocked.txt
list.txt

------------------------------------------------------------
Set up Server:

1. Put ProxyServer.c in any directory in the server machine
2. Put blocked.txt and list.txt in the same directory as ProxyServer.c 
4. Compile with "gcc ProxyServer.c -pthread"
5. Start the server using "./a.out"

------------------------------------------------------------
Set up and Using Client:

6. Open up you favorite web browser
7. Type in 129.120.151.96:9003/www.website.com and hit enter
8. If the website have many images and javascript the browser may timeout
9. Example of working websites include www.google.com, www.delorie.com
10. Wait for the response from the server
11. Websites such as www.facebook.com and www.youtube.com will be blocked
