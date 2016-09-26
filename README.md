httptunnel creates a bidirectional virtual data path tunnelled in HTTP  
requests.  The requests can be sent via an HTTP proxy if so desired.  

This can be useful for users behind restrictive firewalls.  If WWW  
access is allowed through an HTTP proxy, it's possible to use  
httptunnel and, say, telnet or PPP to connect to a computer outside  
the firewall.  

If you still don't understand what this is all about, maybe you  
can find some useful information in the FAQ file.  

This program is mostly intended for technically oriented users.  
They should know what to do.  

Read INSTALL for instructions on how to build a released version.  
If you build the development repository, run ./autogen.sh first.  

httptunnel is free software.  See COPYING for terms and conditions.  
If you like it, I would appreciate if you sent a post card to:  
> Lars Brinkhoff  
> Kopmansgatan 2  
> 411 13  Goteborg  
> Sweden  

Information and/or latest release should be available from these places:  
 * http://www.nocrew.org/software/httptunnel.html  
 * http://www.gnu.org/software/httptunnel/httptunnel.html  
 * ftp://ftp.gnu.org/pub/gnu/httptunnel  

I take no responsibility for what you do with this software.  It has  
the potential to do dangerous things, like disabling the protection  
you system administrator has set up for the local network.  Read the  
DISCLAIMER file.  

There are two programs: hts and htc.  hts is the httptunnel server  
and htc is the client.  hts should be installed on a computer outside  
the HTTP proxy, and htc should be installed on your local computer.  

Documentation about how to use the programs should be searched in this  
order:  
 1. source code  
 2. --help output  
 3. FAQ  
 4. README  

Having said that, here's a (probably outdated) example:  
 * At host REMOTE, start hts like this:  
   `hts -F localhost:23 8888`  
 * At host LOCAL, start htc like this:  
   `htc -F 2323 -P PROXY:8000 REMOTE:8888`  
 * or, if using a buffering HTTP proxy:  
   `htc -F 2323 -P PROXY:8000 -B 48K REMOTE:8888`  
  * Now you can do this at host LOCAL:  
    `telnet localhost 2323`  
    and you will hopefully get a login prompt from host REMOTE.  

See also:  

 * https://sergvergara.files.wordpress.com/2011/04/http_tunnel.pdf - excellent httptunnel tutorial, examples, & info
 * http://sebsauvage.net/punching/ - another excellent example
 * https://daniel.haxx.se/docs/sshproxy.html - more useful info
 * http://neophob.com/2006/10/gnu-httptunnel-v33-windows-binaries/ - httptunnel Win32 binaries
 * [broken link] ~~http://metalab.unc.edu/LDP/HOWTO/mini/Firewall-Piercing.html which is a good introduction to firewall piercing.  It also has describes one way to use httptunnel.~~
