function Redirect() {
    const hostname = window.location.hostname.toLowerCase();
    // Check if the hostname is an IP address (IPv4)
    const isIpAddress = /^(\d{1,3}\.){3}\d{1,3}$/.test(hostname);
    // Check if the hostname is 'localhost' or any subdomain of 'localhost'
    const isLocalhost = hostname === 'localhost' || hostname.endsWith('.localhost');
    // Check if the hostname includes 'esp32.local' or any subdomain of 'esp32.local'
    const isEsp32Local = hostname === 'esp32.local' || hostname.endsWith('.esp32.local');

    if (!isEsp32Local && !isIpAddress && !isLocalhost) {
        // Preserve the path and query string
        const newUrl = `http://esp32.local${window.location.pathname}${window.location.search}`;
        window.location.href = newUrl;
    }
}

async function SubmitFormData(endpoint, data) {
    try {
        const response = await fetch(endpoint, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: new URLSearchParams(data).toString()
        });

        if (!response.ok) {
            const errorText = await response.text();  // Parse the response as plain text
            throw new Error(errorText || `HTTP error! status: ${response.status}`);
        }

        const text = await response.text();  // Parse the response as plain text
        return text;
    } catch (error) {
        console.error('Error:', error);
        return error.message || 'An error occurred while processing your request. Please try again later.';
    }
}

async function uploadFile(fileData, fileName, url) {
    try {
        const formData = new FormData();
        formData.append('file', fileData, fileName);

        const response = await fetch(url, {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            const responseText = await response.text();
            return { success: true, message: 'Datei erfolgreich hochgeladen: ' + responseText };
        } else {
            return { success: false, message: 'Fehler beim Hochladen der Datei. Statuscode: ' + response.status };
        }
    } catch (error) {
        console.error('Fehler beim Hochladen der Datei:', error);
        return { success: false, message: 'Fehler beim Hochladen der Datei: ' + error.message };
    }
}
