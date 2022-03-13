// import swURL from 'sw:./service-worker.js';

// var chartniveau = new Highcharts.Chart({
//   chart:{ renderTo : 'chart-niveau' },
//   title: { text: 'Niveau VL53L1X' },
//   subtitle: {text: 'Using I2C Interface'},
//   series: [{
//     name:  'Niveau',
//     showInLegend: false,
//     type: 'area',
//     fillColor: {
//       linearGradient: {
//         x1: 0,
//         y1: 0,
//         x2: 0,
//         y2: 1
//       },
//       stops: [
//       [0, Highcharts.getOptions().colors[0]],
//       [1, Highcharts.color(Highcharts.getOptions().colors[0]).setOpacity(0).get('rgba')]
//       ]
//     }//,
//     //data: [6,8]
    
//   }],
//   plotOptions: {
//     line: { animation: false,
//       dataLabels: { enabled: true }
//     },
//     series: { color: '#059e8a' }
//   },
//   xAxis: { 
//     type: 'datetime',
//     dateTimeLabelFormats: { second: '%H:%M:%S' }
//   },
//   yAxis: {
//     title: { text: 'Niveau (%)' },
//     //title: { text: 'Temperature (Fahrenheit)' }
//     plotLines: [{
//       value: '18',
//       color: 'green',
//       dashStyle: 'shortdash',
//       width: 2,
//       label: {
//         text: 'Last quarter minimum'
//       }
//     }, {
//       value: '22',
//       color: 'red',
//       dashStyle: 'shortdash',
//       width: 2,
//       label: {
//         text: 'Last quarter maximum'
//       }
//     }]
//   },
//   credits: { enabled: true }
// });

//var chartniveautests;
document.addEventListener('DOMContentLoaded', function () {
  var options = {
    title: { text: 'Niveau VL53L1X' },
    // subtitle: {text: 'Using I2C Interface'},
    // chart: {
    //     type: 'area'
    // },
    time: {
      useUTC: false
    },
    legend: {
      enabled: true
    },
    rangeSelector: {
        buttons: [{
            count: 1,
            type: 'minute',
            text: '1M'
        }, {
            count: 5,
            type: 'minute',
            text: '5M'
        }, {
            type: 'all',
            text: 'All'
        }],
        inputEnabled: false,
        selected: 0
    },
    title: {
        text: 'Niveau (%)'
    },
    plotOptions: {
      series: {
        
        showInNavigator: true
      },
      line: { 
        animation: false,
        dataLabels: { enabled: true }
      },
      // series: [
      //   { 
      //     color: '#059e8a',
      //     name: "Niveau" 
      //   },
      //   { 
      //     color: '#05918a',
      //     name: "Ouverture" 
      //   }
      // ]
    },
    xAxis: {
        //categories: [],
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
      data: [],
      color: '#05918a',
      
      },
      { 
        data:[],
        name: 'Ouverture (%)',
        color: '#FFAA8a' 
      },
      { 
        data:[],
        name: 'Cible (%)',
        color: '#FFC78A' 
      }//, {
    //       name: 'John',
    //       data: [5, 7, 3]
    //   }
    ]
  };
  var optionsrpm = {
    title: { text: 'RPM' },
    subtitle: {text: 'Using I2C Interface'},
    // chart: {
    //     type: 'area'
    // },
    time: {
      useUTC: false
    },
    legend: {
      enabled: true
    },

    rangeSelector: {
        buttons: [{
            count: 1,
            type: 'minute',
            text: '1M'
        }, {
            count: 5,
            type: 'minute',
            text: '5M'
        }, {
            type: 'all',
            text: 'All'
        }],
        inputEnabled: false,
        selected: 0
    },
    title: {
        text: 'Rotation (rpm)'
    },
    plotOptions: {
      line: { 
        animation: false,
        dataLabels: { enabled: true }
      },
      
    },
    xAxis: {
        //categories: [],
      type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
        title: {
            text: '(rpm)'
        }
    },
    series: [ {
        name: 'Rotation (rpm)',
        data: [],
        color: '#b2414a'
      }
    ]
  };
 
  
  Highcharts.ajax({  
    url: 'data.csv',  
    dataType: 'text', 
    success: function(data) {
      // console.log( data)
      // var i = 0;
      // data.data.forEach(element => {
      //   //console.log(element)
      //   //console.log([i,element.niveau])
      //   options.series[0].data.push([(new Date(element.time * 1000)).getTime(), element.niveauEtang* 100 ])
      //   options.series[1].data.push([(new Date(element.time * 1000)).getTime(),element.ouverture*100])
      //   options.series[2].data.push([(new Date(element.time * 1000)).getTime(),element.setpoint*100])
      // });
        // options.series[0].data = data;

      var lines = data.split('\n');
      lines.forEach(function (line,lineNo) {
        var items = line.split(',');
        // header line containes categories  
        if (lineNo == 0) {  
          items.forEach(function(item, itemNo) {  
              //if (itemNo > 0) options.xAxis.categories.push(item);  
          });  

        } 
        // the rest of the lines contain data with their name in the first position  
        else {  
            var series = {   
                data: []  
            };  
            var seriesrpm = {   
              data: []  
          };  
          var dt = new Date(((items[0]-3600)) * 1000).getTime();
          optionsrpm.series[0].data.push([dt,parseFloat(items[1])]);
          options.series[0].data.push([dt,parseFloat(items[2])*100]);
          options.series[1].data.push([dt,parseFloat(items[4])*100]);
          options.series[2].data.push([dt,parseFloat(items[3])*100]);
            // items.forEach(function(item, itemNo) {  
            //     if (itemNo == 0) {  
            //         series.name = item; 
            //         seriesrpm.name = item; 
            //     } else if(itemNo == 1) {  
            //       seriesrpm.data.push(parseFloat(item));  
            //     }  else {
            //       series.data.push(parseFloat(item));  
            //     }
            // });  
              
            // options.series.push(series);  
            // optionsrpm.series.push(seriesrpm);  



        }   
      })
      chartniveautests = Highcharts.stockChart('chart-niveautests', options );
      chartrpm = Highcharts.stockChart('chart-rpm',optionsrpm);
      maj()
      majTimer();
    }  ,
    error: function (e, t) {  
      console.error(e, t);  
    }  
  });
  //maj();
 });

function update(element, action) {
  console.log("update",element,action)
 
  board = element.parentElement.parentElement.parentElement.id
  //toggleLoading(element.parentElement.parentElement.parentElement)
  setLoading(element.parentElement.parentElement.parentElement,true)
  board = board.replace("board-","")
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
          console.log(this.responseText)

      } else{
          //console.log (this.status)
      }
  }
  xhr.open("GET", "/update?b=" + board + "&c="+ action, true);
  xhr.send();
}
function updateb(board, action) {
  console.log(board);
  console.log(action)
  console.log("updateb");
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
          console.log(this.responseText)

      } else{
          //console.log (this.status)
      }
  }
  xhr.open("GET", "/update?b=" + board + "&c="+ action, true);
  xhr.send();
}
function updateSleep(board, mode) {
  SleepTime = document.getElementById("SleepTime").value;
  //updateb(board,mode + "Sleep=" + SleepTime)
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
          console.log(this.responseText)

      } else{
          //console.log (this.status)
      }
  }
  xhr.open("GET", "/update?b=" + board + "&" + mode + "Sleep=" + SleepTime, true);
  xhr.send();
}

/**
 * 
 * @param  el 
 */
function toggleLoading(el){
  console.log("toggleloading",el);
  spinner = el.querySelector(".spinner-border")
  if (spinner.style.display === "none") {
    spinner.style.display = "block";
  } else {
    spinner.style.display = "none";
  }
}
function setLoading(el, state) {
  spinner = el.querySelector(".spinner-border")
  if (state) {
    spinner.style.display = "block";
  } else {
    spinner.style.display = "none";
  
  }
}

/**
 * maj
 */
function maj(){
  
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      console.log(this.responseText)
      try {
        var myObj = JSON.parse( this.responseText);
      console.log(myObj)
      var time = (new Date()).getTime()
      for (i in myObj.boards) {
        x = document.getElementById('board-' + myObj.boards[i].localAddress)
        modal = document.getElementById('modal'+  myObj.boards[i].Name)
        //x.getElementsByClassName("message")[0].innerHTML = JSON.stringify( myObj.boards[i].lastMessage.content)

        modal.getElementsByClassName("message")[0].innerHTML = JSON.stringify( myObj.boards[i].lastMessage.content)
        modal.getElementsByClassName("lastUpdate")[0].innerHTML =  (myObj["msSystem"] - myObj.boards[i].lastUpdate) /1000
        //console.log(myObj.boards[i].lastMessage.content);
        
        if (myObj.boards[i].Name == "Etang") {
          
          var message = myObj.boards[i].lastMessage.content
          
          
          var yn = message.Niveau * 100
          yn =  parseFloat(yn.toFixed(2))

          var barProgress = x.querySelector(".progress-bar")
          barProgress.style.width = yn + '%'

            //ajout donnée niveau dans graph
          // if (chartniveautests.series[0].data.length > 40 ) {
          //   //chartniveau.series[0].addPoint([x, yn],true ,true,true);
          //   chartniveautests.series[0].addPoint([time,yn],true,true,true)
          // } else {
            //chartniveau.series[0].addPoint([x, yn],true ,false,true);
            chartniveautests.series[0].addPoint([time, yn],true ,false,true);
          
          //}
        } else if (myObj.boards[i].Name == "Turbine") {
          var message = myObj.boards[i].lastMessage.content
          var turbineBlock = document.querySelector('#board-11')
          var setpoint = turbineBlock.querySelector('#setPointSlider')
          console.log("setpoint: ",setpoint);
          
          
          var wn = message.Taqui
          wn = parseFloat(wn.toFixed(2))
          var yn = message.Ouverture * 100
          yn = parseFloat(yn.toFixed(2))
          var zn = message.Setpoint * 100
          zn = parseFloat(zn.toFixed(2))

          setpoint.value = zn;
          var barProgress = turbineBlock.querySelector(".progress-bar")
          barProgress.style.width = yn + '%'
            //ajout donnée niveau dans graph
            // if (chartrpm.series[0].data.length > 40 ) {
            //   //chartniveau.series[0].addPoint([x, yn],true ,true,true);
            //   chartrpm.series[0].addPoint([time,wn],true,true,true)
            // } else {
              //chartniveau.series[0].addPoint([x, yn],true ,false,true);
              chartrpm.series[0].addPoint([time, wn],true ,false,true);
            
            //}
          // if (chartniveautests.series[1].data.length > 40 ) {
          //   //chartniveau.series[0].addPoint([x, yn],true ,true,true);
          //   chartniveautests.series[1].addPoint([time,yn],true,true,true)
          // } else {
            //chartniveau.series[0].addPoint([x, yn],true ,false,true);
            chartniveautests.series[1].addPoint([time, yn],true ,false,true);
          
          //}
          // if (chartniveautests.series[2].data.length > 40 ) {
          //   //chartniveau.series[0].addPoint([x, yn],true ,true,true);
          //   chartniveautests.series[2].addPoint([time,zn],true,true,true)
          // } else {
            //chartniveau.series[0].addPoint([x, yn],true ,false,true);
            chartniveautests.series[2].addPoint([time, zn],true ,false,true);
          
          //}
        }
      }
      } catch (error) {
        console.log(error);
      }
      


      
    
    }
  };
  xhr.open("GET", "/maj", true);
  xhr.send();
  setTimeout(maj,5000);
}

function majTimer(){
  Array.from(document.getElementsByClassName("lastUpdate")).forEach(function(item){
    item.innerHTML = parseFloat(item.innerHTML) + 2
  });
  setTimeout(majTimer,2000);
};

// //-------------PWA


// // Register the service worker
// if ('serviceWorker' in navigator) {
//   // Wait for the 'load' event to not block other work
//   window.addEventListener('load', async () => {
//     // Try to register the service worker.
//     try {
//       const reg = await navigator.serviceWorker.register(swURL);
//       console.log('Service worker registered! 😎', reg);
//     } catch (err) {
//       console.log('😥 Service worker registration failed: ', err);
//     }
//   });
// }
// window.addEventListener('DOMContentLoaded', async () => {
//   // Set up the editor
//   const { Editor } = await import('./app/editor.js');
//   const editor = new Editor(document.body);

//   // Set up the menu
//   const { Menu } = await import('./app/menu.js');
//   new Menu(document.querySelector('.actions'), editor);

//   // Set the initial state in the editor
//   const defaultText = `# Welcome to PWA Edit!\n\nTo leave the editing area, press the \`esc\` key, then \`tab\` or \`shift+tab\`.`;

//   editor.setContent(defaultText);
// });

