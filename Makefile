# jekyll docs: https://github.com/mojombo/jekyll/blob/master/README.textile

all: downloadfiles

downloadfiles:
	find download -name '.*' -prune -o -type f -printf "<li><a href=\"/%p\" onClick=\"javascript: _gaq.push(['_trackPageview', '/%p']);\">x%p</a> (%kk)\\n" | sed -e 's|xdownload/||' > _includes/downloadfiles.html


# installation: sudo apt-get install ruby1.8-dev rubygems ruby; gem install jekyll
export PATH:=$(PATH):/var/lib/gems/1.8/bin
test:	downloadfiles
	xterm -e sh -c "jekyll --server --auto; read -p done. -t 5 ok" &
	sleep 5
	sensible-browser http://localhost:4000/ &

clean:
	rm -rf _site/
