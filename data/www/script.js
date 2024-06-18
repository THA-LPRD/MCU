// Führt Funktionen aus, sobald die Seite vollständig geladen ist
window.onload = function() {
    displayIndex();          // Zeigt die Startseite an
    getDisplayDimensions()   // Ruft die Displayabmessungen vom Server ab
}

// Variablen für die Display-Breite und -Höhe
let displayWidth, displayHeight;

// Asynchrone Funktion, um die Displayabmessungen vom Server abzurufen
async function getDisplayDimensions() {
    try {
        // Abrufen der Display-Breite
        let widthResponse = await fetch('/api/v1/GetDisplayWidth');
        if (!widthResponse.ok) {
            throw new Error('Network response was not ok');
        }
        displayWidth = await widthResponse.text();

        // Abrufen der Display-Höhe
        let heightResponse = await fetch('/api/v1/GetDisplayHeight');
        if (!heightResponse.ok) {
            throw new Error('Network response was not ok');
        }
        displayHeight = await heightResponse.text();

        // Aktualisieren der DOM-Elemente mit den abgerufenen Abmessungen
        document.querySelectorAll('.displayWidth').forEach(elem => elem.textContent = displayWidth);
        document.querySelectorAll('.displayHeight').forEach(elem => elem.textContent = displayHeight);

    } catch (error) {
        console.error('Error fetching display dimensions:', error);
        document.getElementById('displayError').textContent = 'Fehler beim Abrufen der Displayabmessungen: ' + error.message;

        // Standardwerte anzeigen, wenn ein Fehler auftritt
        document.querySelectorAll('.displayWidth').forEach(elem => elem.textContent = ' - ');
        document.querySelectorAll('.displayHeight').forEach(elem => elem.textContent = ' - ');
    }
}

// Asynchrone Funktion, um das System neu zu starten
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

// Funktionen zum Anzeigen verschiedener Seitenbereiche
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

// Funktion zum Anzeigen eines Bildes basierend auf der ausgewählten Datei
function displayImage(event) {
    var file = event.target.files[0];
    var reader = new FileReader();
    reader.onload = function(e) {
        var img = document.createElement('img');
        img.src = e.target.result;
        img.style.maxWidth = '100%'; // Beschränke die Breite des Bildes
        var imageContainer = document.getElementById('imageContainer');
        imageContainer.innerHTML = ''; // Entferne vorherige Inhalte
        imageContainer.appendChild(img);
    }
    reader.readAsDataURL(file);
}

// Funktion zum Hochladen einer PNG-Datei
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
            // Überprüfen, ob die Breite und Höhe des Bildes displayWidth x displayHeight sind
            if (img.width !== Number(displayWidth) || img.height !== Number(displayHeight)) {
                alert('Das PNG ist nicht im richtigen Pixelformat für das angeschlossene Display');
                return;
            }

            // Datei hochladen, wenn das Bild die richtigen Abmessungen hat
            uploadFile(file, file.name, '/api/v1/UploadImg')
                .then(result => {
                    if (result.success) {
                        alert(result.message);
                    } else {
                        alert(result.message);
                    }
                })
                .catch(error => {
                    console.error('Error handling file upload:', error);
                });
        };
        img.src = event.target.result;
    };
    reader.readAsDataURL(file);
}

// Funktion zur Umwandlung des Textes in HTML und zur Anzeige in der Vorschau
function convertToHTML() {
    var inputText = document.getElementById("inputText").value;
    var preview = document.getElementById("preview");

    // Set the preview content
    preview.innerHTML = inputText;
}

// Funktion zur Umwandlung des Textes in HTML und zur Anzeige in der Vorschau für ein anderes Textfeld
function convertToHTML2() {
    var inputText = document.getElementById("inputText2").value;
    var preview = document.getElementById("preview2");

    // Set the preview content
    preview.innerHTML = inputText;

    // Aktualisieren eines benutzerdefinierten Textfeldes, falls vorhanden
    try { 
        document.getElementById("customTextfield1").innerHTML = document.getElementById("customInput1").value 
    } catch (error) {}
}

// Funktion zur Skalierung des Bildes in der Vorschau
function scaleImage(previewId, containerId) {
    var preview = document.getElementById(previewId);
    var scaledImageContainer = document.getElementById(containerId);

    // Temporär die Vorschau-Größe anpassen, um den gesamten Inhalt zu erfassen
    var originalWidth = preview.style.width;
    var originalHeight = preview.style.height;

    // Breite und Höhe des Scroll-Inhalts erfassen
    var scrollWidth = preview.scrollWidth;
    var scrollHeight = preview.scrollHeight;

    // Vorschau-Container an den Inhalt anpassen
    preview.style.width = scrollWidth + 'px';
    preview.style.height = scrollHeight + 'px';

    // Verwendung von html2canvas, um den gesamten Inhalt der Vorschau zu erfassen
    html2canvas(preview, { scale: 1 }).then(canvas => {
        var img = document.createElement('img');
        img.src = canvas.toDataURL('image/png');
        img.style.maxWidth = '100%'; // Breite des Bildes beschränken

        // Skalierter Inhalt in den scaledImageContainer setzen
        scaledImageContainer.innerHTML = ''; // Vorherige Inhalte entfernen
        scaledImageContainer.appendChild(img);

        // Originalgrößen wiederherstellen
        preview.style.width = originalWidth;
        preview.style.height = originalHeight;
    });
}

// Funktion zum Laden einer HTML-Vorlage
function loadTemplate() {
    var selectElement = document.getElementById("template");
    var selectedTemplate = selectElement.value;

    // URL zur ausgewählten Vorlagendatei im layouts-Ordner
    var url = './layouts/' + selectedTemplate;

    // Fetch-Anfrage, um die Datei zu laden
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

// Funktion, um Event-Listener für Eingabefelder zu setzen
function setupInputListener(inputId, previewId, customMapping) {
    document.getElementById(inputId).addEventListener('input', () => {
        convertAndUploadHtml(previewId, inputId, customMapping);
    });
}

// Setup der Event-Listener, sobald das DOM vollständig geladen ist
document.addEventListener('DOMContentLoaded', () => {
    setupInputListener('inputText', 'preview');
    setupInputListener('inputText2', 'preview2', [{inputId: 'customInput1', outputId: 'customTextfield1'}]);
});

// Funktion zur Umwandlung und zum Hochladen von HTML-Inhalten
function convertAndUploadHtml(previewId, inputTextId) {
    var inputText = document.getElementById(inputTextId).value;
    var preview = document.getElementById(previewId);

    // Temporär die Vorschau-Größe anpassen, um den gesamten Inhalt zu erfassen
    var originalWidth = preview.style.width;
    var originalHeight = preview.style.height;

    // Breite und Höhe des Scroll-Inhalts erfassen
    var scrollWidth = preview.scrollWidth;
    var scrollHeight = preview.scrollHeight;

    // Vorschau-Container an den Inhalt anpassen
    preview.style.width = scrollWidth + 'px';
    preview.style.height = scrollHeight + 'px';

    // Verwendung von html2canvas, um den gesamten Inhalt der Vorschau zu erfassen
    html2canvas(preview, { scale: 1 }).then(canvas => {
        // Überprüfen, ob das Bild die richtigen Abmessungen hat
        if (canvas.width !== Number(displayWidth) || canvas.height !== Number(displayHeight)) {
            alert('Ihr Design ist nicht im richtigen Pixelformat für das angeschlossene Display');
            return;
        }

        // Erstellen eines Blob-Objekts aus dem Canvas-Bild
        canvas.toBlob(function(blob) {
            // Datei hochladen
            uploadFile(blob, "html_conversion.png", '/api/v1/UploadImg')
                .then(result => {
                    if (result.success) {
                        alert(result.message);
                    } else {
                        alert(result.message);
                    }
                })
                .catch(error => {
                    console.error('Error handling file upload:', error);
                });
        }, 'image/png');

        // Originalgrößen wiederherstellen
        preview.style.width = originalWidth;
        preview.style.height = originalHeight;
    });
}
