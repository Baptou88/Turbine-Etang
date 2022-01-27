function urlencodeFormData(fd){
    var s = '';
    function encode(s){ return encodeURIComponent(s).replace(/%20/g,'+'); }
    for(var pair of fd.entries()){
        if(typeof pair[1]=='string'){
            s += (s?'&':'') + encode(pair[0])+'='+encode(pair[1]);
        }
    }
    return s;
}

// document.querySelectorAll('form').forEach(function (element){
//     console.log(element);
//     element.addEventListener('submit', function(e) {
//     e.preventDefault();
//     let formData = new FormData(this);
//     console.log(formData);
//     let parsedData = {};
//     for(let name of formData) {
//         console.log(name);
//       if (typeof(parsedData[name[0]]) == "undefined") {
//         let tempdata = formData.getAll(name[0]);
//         if (tempdata.length > 1) {
//           parsedData[name[0]] = tempdata;
//         } else {
//           parsedData[name[0]] = tempdata[0];
//         }
//       }
//     }

//     let options = {};
//     switch (this.method.toLowerCase()) {
//       case 'post':
//         options.body = JSON.stringify(parsedData);
//       case 'get':
//         options.method = this.method;
//         options.headers = {'Content-Type': 'application/json'};
//       break;
//     }
//     console.log(options);
//     fetch(this.action, options).then(r => r.json()).then(data => {
//       console.log(data);
//     });
// });
// })

document.querySelectorAll('form').forEach(function (element){
    console.log(element);
    element.addEventListener('submit', function(e) {
    e.preventDefault();
    let formData = new FormData(this);
    console.log(formData);
    
    
    
    
    let parsedData = {};
    for(let name of formData) {
        console.log(name);
      if (typeof(parsedData[name[0]]) == "undefined") {
        let tempdata = formData.getAll(name[0]);
        if (tempdata.length > 1) {
          parsedData[name[0]] = tempdata;
        } else {
          parsedData[name[0]] = tempdata[0];
        }
      }
    }

    let options = {};
    switch (this.method.toLowerCase()) {
      case 'post':
        options.body = urlencodeFormData(formData);
      case 'get':
        options.method = this.method;
        options.headers = {'Content-Type':'application/x-www-form-urlencoded'};
      break;
    }
    console.log(options);
    fetch(this.action, options).then(r => r.json()).then(data => {
      console.log(data);
    });
    
    
});
})
    