# About
httptunnel creates a bidirectional virtual data path tunnelled in HTTP  
requests.  The requests can be sent via an HTTP proxy if so desired.  

This can be useful for users behind restrictive firewalls.  If WWW  
access is allowed through an HTTP proxy, it's possible to use  
httptunnel and, say, telnet or PPP to connect to a computer outside  
the firewall.  

If you still don't understand what this is all about, maybe you  
can find some useful information in the FAQ file.  

This program is mostly intended for technically-oriented users.  
They should know what to do.  

# Install
Read [INSTALL](./INSTALL) for instructions on how to build a released version.  
If you build the development repository, run `./autogen.sh` first.  

# License
httptunnel is free software.  See COPYING for terms and conditions.  
If you like it, I would appreciate if you sent a post card to:  
> Lars Brinkhoff  
> Bokskogsbacken 66
> 422 56  Goteborg  
> Sweden  

Information and/or latest release should be available from these places:  
 * https://github.com/larsbrinkhoff/httptunnel
 * http://www.gnu.org/software/httptunnel/httptunnel.html  
 * ftp://ftp.gnu.org/pub/gnu/httptunnel  

I take no responsibility for what you do with this software.  It has  
the potential to do dangerous things, like disabling the protection  
your system administrator has set up for the local network.  Read the  
DISCLAIMER file.  

# Usage & Documentation
There are two programs: `hts` and `htc`.  `hts` is the *httptunnel server*  
and `htc` is the *client*.  `hts` should be installed on a computer outside  
the HTTP proxy, and `htc` should be installed on your local computer.  

Documentation about how to use the programs should be searched in this  
order:  
 1. source code  
 2. --help output  
 3. FAQ  
 4. README  

Having said that, here are some examples:  
 * start httptunnel server:  
  * At host REMOTE, start `hts` like this:  
    `hts -F localhost:23 8888` (set up httptunnel server to listen on port 8888 and forward to localhost:23)   
 * start httptunnel client:  
   * At host LOCAL, start `htc` like this:  
    `htc -F 2323 -P PROXY_ADDRESS:8000 REMOTE_IP:8888` (set up httptunnel client to forward localhost:2323 to REMOTE_IP:8888 via a local proxy at PROXY_ADDRESS:8000) 
  * or, if using a buffering HTTP proxy:  
    `htc -F 2323 -P PROXY_ADDRESS:8000 -B 48K REMOTE_IP:8888`  
  * Now you can do this at host LOCAL:  
    `telnet localhost 2323` (telnet in to REMOTE_IP:8888 via your httptunnel you just configured above on port localhost:2323)  
    ...and you will hopefully get a login prompt from host REMOTE_IP.  
 * Debugging:
  * For debug output, add `-Dn` to the end of a command, where `n` is the level of debug output you'd like to see, with 0 meaning no
	debug messages at all, and 5 being the highest level (verbose).  
  * ex: `htc -F 10001 -P PROXY_ADDRESS:8000 REMOTE_IP:8888 -D5` will show verbose debug output (level 5 debugging) while setting up an httptunnel client to forward localhost:10001 to REMOTE_IP:8888 via a local proxy at PROXY_ADDRESS:8000

# External help, examples, & links

 * https://sergvergara.files.wordpress.com/2011/04/http_tunnel.pdf - excellent httptunnel tutorial, examples, & info
 * http://sebsauvage.net/punching/ - another excellent example
 * https://daniel.haxx.se/docs/sshproxy.html - more useful info
 * http://neophob.com/2006/10/gnu-httptunnel-v33-windows-binaries/ - httptunnel Win32 binaries (download here)
 * [Google search for "http tunnel v3.3"](https://www.google.com/webhp?sourceid=chrome-instant&ion=1&espv=2&ie=UTF-8#q=http%20tunnel%20v3.3) - brings up lots of good links to httptunnel (this search seems to work better than searching for "httptunnel" alone since the latter brings up many generic search results or results pertaining to other tools)
