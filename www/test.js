(function () {
    function myplot(){
        // Set the dimensions of the canvas / graph
        var margin = {top: 30, right: 20, bottom: 30, left: 50},
            width = 600 - margin.left - margin.right,
            height = 270 - margin.top - margin.bottom;
        // Set the ranges
        var x = d3.scale.linear().range([0, width]);
        var y = d3.scale.linear().range([height, 0]);

        // Define the axes
        var xAxis = d3.svg.axis().scale(x)
            .orient("bottom").ticks(5);

        var yAxis = d3.svg.axis().scale(y)
            .orient("left").ticks(5);

        // Define the line
        var valueline = d3.svg.line()
            .x(function(d) { return x(d.date); })
            .y(function(d) { return y(d.close); });
        
        // Adds the svg canvas
        var svg = d3.select("#indata")
            .append("svg")
            .attr("width", width + margin.left + margin.right)
            .attr("height", height + margin.top + margin.bottom)
            .append("g")
            .attr("transform", 
                  "translate(" + margin.left + "," + margin.top + ")");
        
        // Scale the range of the data
        x.domain([]);
        y.domain([]);

        // Add the valueline path.
        svg.append("path")
            .attr("class", "line")
            .attr("d", valueline([]));
        
        // Add the X Axis
        svg.append("g")
            .attr("class", "x axis")
            .attr("transform", "translate(0," + height + ")")
            .call(xAxis);

        // Add the Y Axis
        svg.append("g")
            .attr("class", "y axis")
            .call(yAxis);        

        var inter = setInterval(function() {
            var sec = Math.trunc(Date.now()/1000);
            document.getElementById("demo").innerHTML = sec;
            if( sec%2 == 0 ){
                x.domain([]);
	        y.domain([]);
                var svg = d3.select("#indata").transition();
                svg.select(".line")   // change the line
                    .attr("d", valueline([]));
                svg.select(".x.axis") // change the x axis
                    .call(xAxis);
                svg.select(".y.axis") // change the y axis
                    .call(yAxis);
            }
            else{
                var data_v = d3.range(40).map(function(i) {
                    return {date: i, close: (Math.sin(i / 3) + 2) / 4};
                });
    	        x.domain(d3.extent(data_v, function(d) { return d.date; }));
	        y.domain([0, d3.max(data_v, function(d) { return d.close; })]);
                var svg = d3.select("#indata").transition();
                svg.select(".line")   // change the line
                    .attr("d", valueline(data_v));
                svg.select(".x.axis") // change the x axis
                    .call(xAxis);
                svg.select(".y.axis") // change the y axis
                    .call(yAxis);
            }
            
        }, 1000); 
    }
    


    function get_appropriate_ws_url(extra_url)
    {
	var pcol;
	var u = document.URL;

	/*
	 * We open the websocket encrypted if this page came on an
	 * https:// url itself, otherwise unencrypted
	 */

	if (u.substring(0, 5) === "https") {
	    pcol = "wss://";
	    u = u.substr(8);
	} else {
	    pcol = "ws://";
	    if (u.substring(0, 4) === "http")
		u = u.substr(7);
	}

	u = u.split("/");
	/* + "/xxx" bit is for IE10 workaround */
	return pcol + u[0] + "/" + extra_url;
    }

    var socket_di;

    function ws_open_dumb_increment()
    {
	socket_di = new WebSocket(get_appropriate_ws_url(""), "dumb-increment-protocol");
	try {
	    socket_di.onopen = function() {
		document.getElementById("wsdi_statustd").style.backgroundColor =
		    "#40ff40";
		document.getElementById("wsdi_status").innerHTML =
		    " <b>websocket connection opened</b><br>" +
		    socket_di.extensions;
	    };

	    socket_di.onmessage =function got_packet(msg) {
		document.getElementById("number").textContent = msg.data + "\n";
	    };

	    socket_di.onclose = function(){
		document.getElementById("wsdi_statustd").style.backgroundColor =
		    "#ff4040";
		document.getElementById("wsdi_status").textContent =
		    " websocket connection CLOSED ";
	    };
	} catch(exception) {
	    alert("<p>Error" + exception);  
	}
    }
    
    function reset() {
	socket_di.send("reset\n");
    }

    function junk() {
	for(var word = ""; word.length < 9000; word += "a"){}
	socket_di.send(word);
    }

    myplot();

    var socket_ot;
    window.addEventListener("load", function() {
	document.getElementById("offset").onclick = reset;
	// document.getElementById("junk").onclick = junk;
	var transport_protocol = "";
	if ( performance && performance.timing.nextHopProtocol ) {
	    transport_protocol = performance.timing.nextHopProtocol;
	} else if ( window.chrome && window.chrome.loadTimes ) {
	    transport_protocol = window.chrome.loadTimes().connectionInfo;
	} else {

	    var p = performance.getEntriesByType("resource");
	    for (var i=0; i < p.length; i++) {
	        var value = "nextHopProtocol" in p[i];
	        if (value)
	            transport_protocol = p[i].nextHopProtocol;
	    }
	}
	
        console.log("transport protocol " + transport_protocol);
	
        if (transport_protocol === "h2")
	    document.getElementById("transport").innerHTML =
	    "<img src=\"./http2.png\">";

        document.getElementById("number").textContent =
	    get_appropriate_ws_url("");
        
        ws_open_dumb_increment();
        
    }, false);

    
}());
