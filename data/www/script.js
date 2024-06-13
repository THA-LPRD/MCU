window.onload = function() {
    displayIndex();
    getDisplayDimensions()
}

  
let displayWidth, displayHeight;

   async function getDisplayDimensions() {
    try {
        let widthResponse = await fetch('/api/v1/GetDisplayWidth');
        if (!widthResponse.ok) {
            throw new Error('Network response was not ok');
        }
        displayWidth = await widthResponse.text();

        let heightResponse = await fetch('/api/v1/GetDisplayHeight');
        if (!heightResponse.ok) {
            throw new Error('Network response was not ok');
        }
        displayHeight = await heightResponse.text();


        // Update the DOM elements with the fetched dimensions
        document.querySelectorAll('.displayWidth').forEach(elem => elem.textContent = displayWidth);
        document.querySelectorAll('.displayHeight').forEach(elem => elem.textContent = displayHeight);

    } catch (error) {
        console.error('Error fetching display dimensions:', error);
        document.getElementById('displayError').textContent = 'Fehler beim Abrufen der Displayabmessungen: ' + error.message;

        // Display default width and height
        document.querySelectorAll('.displayWidth').forEach(elem => elem.textContent = ' - ');
        document.querySelectorAll('.displayHeight').forEach(elem => elem.textContent = ' - ');
    }
   }

   async function restart() {
    try {
    const response = await fetch('/api/v1/restart', {
    method: 'POST'
    });

    if (!response.ok) {
    throw new Error('Network response was not ok');
    }

    return 'Neustart erfolgreich!';
    } catch (error) {
    console.error('Error during restart:', error);
    return 'Neustart fehlgeschlagen.';
    }
   }

   function displayIndex() {
    document.getElementById("htmlVorlage").style.display = "none";
    document.getElementById("htmlDesign").style.display = "none";
    document.getElementById("pngUpload").style.display = "none"; 
    document.getElementById("intro").style.display = "block";
   }

   function displayPngUpload() {
    document.getElementById("htmlVorlage").style.display = "none";
    document.getElementById("intro").style.display = "none";
    document.getElementById("htmlDesign").style.display = "none"; 
    document.getElementById("pngUpload").style.display = "block"; 
   }

   function displayHtmlDesign() {
    document.getElementById("pngUpload").style.display = "none"
    document.getElementById("intro").style.display = "none";
    document.getElementById("htmlVorlage").style.display = "none"; 
    document.getElementById("htmlDesign").style.display = "block"; 
   }

    function displayHtmlVorlage() {
    document.getElementById("pngUpload").style.display = "none"
    document.getElementById("intro").style.display = "none";
    document.getElementById("htmlDesign").style.display = "none"; 
    document.getElementById("htmlVorlage").style.display = "block"; 
   }


   function displayImage(event) {
    var file = event.target.files[0];
    var reader = new FileReader();
    reader.onload = function(e) {
        var img = document.createElement('img');
        img.src = e.target.result;
        img.style.maxWidth = '100%'; // Optional: Beschränke die Breite des Bildes
        var imageContainer = document.getElementById('imageContainer');
        imageContainer.innerHTML = ''; // Entferne vorherige Inhalte
        imageContainer.appendChild(img);
    }
    reader.readAsDataURL(file);
   }



   function pngUpload() {
    var fileInput = document.getElementById('fileInput');

    // Überprüfen, ob eine Datei ausgewählt wurde
    if (fileInput.files.length === 0) {
        alert('Bitte wählen Sie eine PNG-Datei aus.');
        return;
    }

    var file = fileInput.files[0];

    // Überprüfen, ob es sich um eine PNG-Datei handelt
    if (file.type !== 'image/png') {
        alert('Bitte wählen Sie eine PNG-Datei aus.');
        return;
    }

        // Verwenden eines FileReader, um die Bilddatei zu lesen
        var reader = new FileReader();
        reader.onload = function(event) {
            var img = new Image();
            img.onload = function() {
                const displayWidth = 800;
                const displayHeight = 480;

                // Überprüfen, ob die Breite und Höhe des Bildes 800x480px sind
                if (img.width !== displayWidth || img.height !== displayHeight) {
                    alert('Das PNG ist nicht im richtigen Pixelformat für das angeschlossene Display');
                    return;
                }

                // Datei hochladen, wenn das Bild die richtigen Abmessungen hat
                uploadFile(file.name, file);
            };
            img.src = event.target.result;
        };
        reader.readAsDataURL(file);
   }


   function convertToHTML() {
        var inputText = document.getElementById("inputText").value;
        var preview = document.getElementById("preview");

        // Set the preview content
        preview.innerHTML = inputText;
   }


   function convertToHTML2() {
    var inputText = document.getElementById("inputText2").value;
    var preview = document.getElementById("preview2");

    // Set the preview content
    preview.innerHTML = inputText;
    try { 
        document.getElementById("customTextfield1").innerHTML = document.getElementById("customInput1").value 
    } catch (error) {}

    }

   
   function loadTemplate() {
    var selectElement = document.getElementById("template");
    var selectedTemplate = selectElement.value;

    // URL zur ausgewählten Vorlagendatei im layouts-Ordner
    var url = './layouts/' + selectedTemplate;

    // Fetch-Anfrage um die Datei zu laden
    fetch(url)
        .then(function(response) {
            // Überprüfen, ob die Anfrage erfolgreich war (Statuscode 200)
            if (!response.ok) {
                throw new Error('Fehler beim Laden der Datei. Statuscode: ' + response.status);
            }
            return response.text(); // Rückgabe des Textinhalts der Antwort
        })
        .then(function(data) {
            // Den Inhalt der Textarea einfügen
            document.getElementById("inputText2").value = data;
        })
        .catch(function(error) {
            console.error('Fehler beim Laden der Datei:', error);
            document.getElementById("inputText2").value = "Fehler beim Laden der Vorlage.";
        });
   }



   document.getElementById('inputText2').addEventListener('input', () => {
       const inputText = document.getElementById("inputText2").value;
       const preview = document.getElementById("preview2");
       preview.innerHTML = inputText;
    });

   
   document.getElementById('inputText').addEventListener('input', () => {
        const inputText = document.getElementById("inputText").value;
        const preview = document.getElementById("preview");
        preview.innerHTML = inputText;
   });

   function convertAndUploadHtml(previewId, inputTextId) {
    var inputText = document.getElementById(inputTextId).value;
    var preview = document.getElementById(previewId);

    html2canvas(preview, { scale: 1 }).then(canvas => {
        if (canvas.width !== Number(displayWidth) || canvas.height !== Number(displayHeight)) {
            alert('Ihr Design ist nicht im richtigen Pixelformat für das angeschlossene Display');
            return;
        }

        // Erstellen eines Blob-Objekts aus dem Canvas-Bild
        canvas.toBlob(function(blob) {
            sendFlag();
            // Datei hochladen
            uploadFile('html_conversion.png', blob).catch(error => {
                console.error('Fehler beim Hochladen der Datei:', error);
            });
        }, 'image/png');
    });
   }

   async function uploadFile(fileName, fileBlob) {
    try {
        let formData = new FormData();
        formData.append('file', fileBlob, fileName);

        let response = await fetch('/api/v1/UploadImg', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            let responseText = await response.text();
            alert('Datei erfolgreich hochgeladen: ' + responseText);
        } else {
            alert('Fehler beim Hochladen der Datei. Statuscode: ' + response.status);
        }
    } catch (error) {
        console.error('Fehler beim Hochladen der Datei:', error);
    }
   }

   function sendFlag() {
    var ditheringCheckbox = document.getElementById('ditheringCheckbox');
    var flag = ditheringCheckbox.checked ? 'on' : 'off';

    var xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/v1/UploadFlag', true);
    xhr.setRequestHeader('Content-Type', 'application/json;charset=UTF-8');

    xhr.onreadystatechange = function() {
        if (xhr.readyState === XMLHttpRequest.DONE) {
            if (xhr.status === 200) {
                alert('Flag erfolgreich gesendet: ' + xhr.responseText);
            } else {
                alert('Fehler beim Senden des Flags. Statuscode: ' + xhr.status);
            }
        }
    };

    var data = {
        flag: flag
    };

    xhr.send(JSON.stringify(data));
   }

