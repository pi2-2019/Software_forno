const char MAIN_page[] PROGMEM = R"=====(
<!doctype html>
<html>
 
<head>
  <title>Estacao de Solda | Controle de Temperatura - Forno </title>
  <!--For offline ESP graphs see this tutorial https://circuits4you.com/2018/03/10/esp8266-jquery-and-ajax-web-server/ -->
  
  <script src = "https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.7.3/Chart.min.js"></script>  
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js"></script>

  <style>
  canvas{
    -moz-user-select: none;
    -webkit-user-select: none;
    -ms-user-select: none;
  }
 
  /* Data Table Styling */
  #dataTable {
    font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;
    border-collapse: collapse;
    width: 100%;
  }
 
  #dataTable td, #dataTable th {
    border: 1px solid #ddd;
    padding: 8px;
  }
 
  #dataTable tr:nth-child(even){background-color: #f2f2f2;}
 
  #dataTable tr:hover {background-color: #ddd;}
 
  #dataTable th {
    padding-top: 12px;
    padding-bottom: 12px;
    text-align: left;
    background-color: #4CAF50;
    color: white;
  }

  .iniciar {
    background-color: #e7e7e7; /* Green */
    border-radius: 12px;
    color: black;
    padding: 15px 32px;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 16px;
    margin-left: 602px;
    padding-top: 15px;
    margin-top: 20px;
    margin-bottom: 20px;
    max-width: 400px;
    margin: 0 auto;
    border: 1px solid #ccc;
  }

  .parar {
    background-color: #e7e7e7; /* Green */
    border-radius: 12px;
    color: black;
    padding: 15px 32px;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 16px;
    margin-left: 602px;
    padding-top: 15px;
    margin-top: 20px;
    margin-bottom: 20px;
    max-width: 400px;
    margin: 0 auto;
    border: 1px solid #ccc;
  }

  #dialog  {
    color: red;
    font-size: 30px;
    text-align: center;
  }
 
  </style>
</head>
 
<body>
    <div style="text-align:center;"><b>Estacao de Solda - PI2 </b><br>Controle de temperatura - Forno</div>
    
    <div style="display: flex">
      <button type="button" id="button1"data-text-swap="INICIADO" class="iniciar" onClick="uploadChart();">Iniciar</button>
      <button class="parar" onClick="resetChart();">Atualizar Pagina</button>
    </div>
    
    <div id="dialog" title="Alerta">
      <p>Abrir a porta do Forno!</p>
    </div>

    <div class="chart-container" position: relative; height:350px; width:100%">
        <canvas id="Chart" width="400" height="400"></canvas>
    </div>
    
    <div>
      <table id="dataTable">
        <tr><th>Tempo</th><th>Valor de Leitura</th></tr>
      </table>
    </div>

<br>
<br>  
 
<script>

//Graphs visit: https://www.chartjs.org
var values = [];
var timeStamp = [];
function showGraph()
{
    for (i = 0; i < arguments.length; i++) {
      values.push(arguments[i]);    
    }
 
    var ctx = document.getElementById("Chart").getContext('2d');
    var Chart2 = new Chart(ctx, {
        type: 'line',
        data: {
            labels: timeStamp,  //Bottom Labeling
            datasets: [{
                label: "Temperatura (Graus Celsius)",
                fill: false,  //Try with true
                backgroundColor: 'rgba( 243, 156, 18 , 1)', //Dot marker color
                borderColor: 'rgba( 243, 156, 18 , 1)', //Graph Line Color
                data: values,
            }],
        },
        options: {
            title: {
                    display: false,
                    text: "Leitura"
                },
            maintainAspectRatio: false,
            elements: {
            line: {
                    tension: 0.5 //Smoothening (Curved) of data lines
                }
            },
            scales: {
                    yAxes: [{
                        ticks: {
                            beginAtZero:true
                        }
                    }]
            }
        }
    });
 
}
 
//On Page load show graphs
window.onload = function() {
  console.log(new Date().toLocaleTimeString());
  showGraph(5,10,4,58);

};


function uploadChart() {

  var button = $("#button1");
  button.text(button.data("text-swap"));
  
  var xhttp = new XMLHttpRequest();

  xhttp.open("POST", "init", true); 
  xhttp.send();
  
  setInterval(function() {
      // Call a function repetatively with 1 Second interval
      getData();
    }, 5500); //1000mSeconds update rate 

}

function resetChart() {

  location.reload();

}

$("#dialog" ).hide();
 
function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
     //Push the data in array
      var time = new Date().toLocaleTimeString();
      var ADCValue = this.responseText; 
      values.push(ADCValue);
      timeStamp.push(time);

      showGraph();  //Update Graphs    
      //Update Data Table
      var table = document.getElementById("dataTable");
      var row = table.insertRow(1); //Add after headings
      var cell1 = row.insertCell(0);
      var cell2 = row.insertCell(1);
      cell1.innerHTML = time;
      cell2.innerHTML = ADCValue;
      
    }
    if(parseInt(ADCValue) > 238) {
      console.log("teste");
      $("#dialog" ).show();
    }
  };
  xhttp.open("GET", "readADC", true); //Handle readADC server on ESP8266
  xhttp.send();
}

    
</script>
</body>
 
</html>
 
)=====";
