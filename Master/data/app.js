var chartniveau = new Highcharts.Chart({
  chart:{ renderTo : 'chart-niveau' },
  title: { text: 'Niveau VL53L1X' },
  subtitle: {text: 'Using I2C Interface'},
  series: [{
    name:  'Niveau',
    showInLegend: false,
    type: 'area',
    fillColor: {
      linearGradient: {
        x1: 0,
        y1: 0,
        x2: 0,
        y2: 1
      },
      stops: [
      [0, Highcharts.getOptions().colors[0]],
      [1, Highcharts.color(Highcharts.getOptions().colors[0]).setOpacity(0).get('rgba')]
      ]
    },
    data: [6,8]
    
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#059e8a' }
  },
  xAxis: { 
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Niveau (%)' },
    //title: { text: 'Temperature (Fahrenheit)' }
    plotLines: [{
      value: '18',
      color: 'green',
      dashStyle: 'shortdash',
      width: 2,
      label: {
        text: 'Last quarter minimum'
      }
    }, {
      value: '22',
      color: 'red',
      dashStyle: 'shortdash',
      width: 2,
      label: {
        text: 'Last quarter maximum'
      }
    }]
  },
  credits: { enabled: true }
});

var chartniveautests;
document.addEventListener('DOMContentLoaded', function () {
  var options = {
    chart: {
        type: 'line'
    },
    title: {
        text: 'Niveau (%)'
    },
    xAxis: {
        categories: [],
        type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
        title: {
            text: '(%)'
        }
    },
    series: [ {
      name: 'niveau (%)',
      data: []
     }//, {
    //      name: 'John',
    //      data: [5, 7, 3]
    //  }
    ]
  };
  
  Highcharts.ajax({  
    url: 'data.json',  
    success: function(data) {
      //console.log(data)
      var i = 0;
      data.forEach(element => {
        //console.log(element)
        //console.log([i,element.niveau])
        options.series[0].data.push([(new Date()).getTime(),element.niveau])
        
      });
        // options.series[0].data = data;
        chartniveautests = Highcharts.chart('chart-niveautests', options );
      }  ,
      error: function (e, t) {  
        console.error(e, t);  
    }  
  });
  maj();
});
function update(element, action) {
    console.log(element)
    console.log(action)
    board = element.parentElement.id
    board = board.replace("board-","")
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            console.log(this.responseText)

        } else{
            console.log (this.status)
        }
    }
    xhr.open("GET", "/update?b=" + board + "&c="+ action, true);
    xhr.send();
}
/**
 * maj
 */
function maj(){
  console.log("er")
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
        
      var myObj = JSON.parse( this.responseText);
      for (i in myObj.boards) {
        x = document.getElementById('board-' + myObj.boards[i].Name)
        x.getElementsByClassName("message")[0].innerHTML = myObj.boards[i].lastMessage.content
        //console.log(x);
      }
      //console.log(myObj);


      var x = (new Date()).getTime()
      var yn = Math.floor((Math.random() * 10) + 1);


        //ajout donnée niveau dans graph
      if (chartniveau.series[0].data.length > 40 ) {
        chartniveau.series[0].addPoint([x, yn],true ,true,true);
        chartniveautests.series[0].addPoint([x,yn],true,true,true)
      } else {
        chartniveau.series[0].addPoint([x, yn],true ,false,true);
        chartniveautests.series[0].addPoint([x, yn],true ,false,true);
      
      }
    
    }
  };
  xhr.open("GET", "/maj", true);
  xhr.send();
  setTimeout(maj,10000);
}



