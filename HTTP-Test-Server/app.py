from flask import Flask, request, jsonify, send_file, send_from_directory
import os
import uuid
import logging

app = Flask(__name__)

UPLOAD_FOLDER = '../data/upload'
WWW_FOLDER = '../data/www/'
OPERATING_MODE = 'Standalone'
DISPLAY_WIDTH = 800
DISPLAY_HEIGHT = 480
DISPLAY_MODULE = "WS_7IN3G"


if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)


@app.route('/')
def index():
    return send_from_directory(WWW_FOLDER, 'index.html')


@app.route('/api/v1/UploadImg', methods=['POST'])
def upload_img():
    if 'file' not in request.files:
        logger.error("[HTTPServer] No file part")
        return "No file part", 400

    file = request.files['file']

    if file.filename == '':
        logger.error("[HTTPServer] No selected file")
        return "No selected file", 400

    file_path = os.path.join(UPLOAD_FOLDER, file.filename)

    try:
        file.save(file_path)
        logger.info(f"[HTTPServer] Image upload successful to {file_path}")
        return "Image successfully uploaded", 200
    except Exception as e:
        logger.error(f"[HTTPServer] Image upload failed: {e}")
        return "Image upload failed", 500

@app.route('/upload/<filename>')
def uploaded_file(filename):
    return send_file(os.path.join(UPLOAD_FOLDER, filename), mimetype='image/png')


@app.route('/<path:filename>')
def serve_www(filename):
    return send_from_directory(WWW_FOLDER, filename)


@app.route('/api/v1/SetOpMode', methods=['POST'])
def set_op_mode():
    global OPERATING_MODE

    client_ip = request.remote_addr
    request_body = request.data.decode('utf-8')

    logger.info(f"[HTTPServer] Received ConfigOpMode request from client {client_ip}")
    logger.debug(f"[HTTPServer] Body: {request_body}")

    if 'opmode' not in request.form:
        logger.debug("[HTTPServer] Invalid request, missing parameters")
        return "Missing Operating Mode", 400

    opmode = request.form['opmode']
    if opmode not in ["valid_mode_1", "valid_mode_2"]:  # Replace with actual valid modes
        logger.error("Invalid Operating Mode")
        return "Invalid Operating Mode", 400

    OPERATING_MODE = opmode
    logger.debug(f"[HTTPServer] Operating Mode set to {OPERATING_MODE}")

    return "Operating Mode set. Restart is needed to apply changes.", 200


@app.route('/api/v1/SetWiFiCred', methods=['POST'])
def set_wifi_cred():
    client_ip = request.remote_addr
    request_body = request.data.decode('utf-8')

    logger.info(f"[HTTPServer] Received SetWiFiCred request from client {client_ip}")
    logger.debug(f"[HTTPServer] Body: {request_body}")

    if 'ssid' not in request.form or 'password' not in request.form:
        logger.debug("[HTTPServer] Invalid request, missing parameters")
        return "Missing SSID or password", 400

    ssid = request.form['ssid']
    password = request.form['password']

    # Simulate setting configuration values
    if not set_config('WiFiSSID', ssid) or not set_config('WiFiPassword', password):
        logger.error("[HTTPServer] Invalid WiFi credentials")
        return "Invalid WiFi credentials", 400

    logger.debug(f"[HTTPServer] WiFi credentials set to {ssid}, {password}")

    # Simulate saving configuration
    save_config()

    return "WiFi credentials set. Restart is needed to apply changes.", 200


@app.route('/api/v1/SetDisplayModule', methods=['POST'])
def set_display_module():
    client_ip = request.remote_addr
    request_body = request.data.decode('utf-8')

    logger.info(f"[HTTPServer] Received SetDisplayModule request from client {client_ip}")
    logger.debug(f"[HTTPServer] Body: {request_body}")

    if 'displayModule' not in request.form:
        logger.debug("[HTTPServer] Invalid request, missing parameters")
        return "Missing Display Module", 400

    display_module = request.form['displayModule']

    # Simulate setting the display module configuration
    if not set_config('DisplayDriver', display_module):
        logger.error("[HTTPServer] Invalid Display Module")
        return "Invalid Display Module", 400

    logger.debug(f"[HTTPServer] Display Module set to {display_module}")

    # Simulate saving the configuration
    save_config()

    return "Display Module set. Restart is needed to apply changes.", 200


@app.route('/api/v1/GetOpMode', methods=['GET'])
def get_op_mode():
    client_ip = request.remote_addr
    request_body = request.data.decode('utf-8') if request.data else ''

    logger.info(f"[HTTPServer] Received GetOpMode request from client {client_ip}")
    logger.debug(f"[HTTPServer] Body: {request_body}")

    # Using the global OPERATING_MODE variable
    global OPERATING_MODE
    op_mode = OPERATING_MODE

    return op_mode, 200


@app.route('/api/v1/GetDisplayWidth', methods=['GET'])
def get_display_width():
    client_ip = request.remote_addr
    request_body = request.data.decode('utf-8') if request.data else ''

    logger.info(f"[HTTPServer] Received GetDisplayWidth request from client {client_ip}")
    logger.debug(f"[HTTPServer] Body: {request_body}")

    # Using the global DISPLAY_WIDTH variable
    global DISPLAY_WIDTH
    display_width = str(DISPLAY_WIDTH)

    return display_width, 200


@app.route('/api/v1/GetDisplayHeight', methods=['GET'])
def get_display_height():
    client_ip = request.remote_addr
    request_body = request.data.decode('utf-8') if request.data else ''

    logger.info(f"[HTTPServer] Received GetDisplayHeight request from client {client_ip}")
    logger.debug(f"[HTTPServer] Body: {request_body}")

    # Using the global DISPLAY_WIDTH variable
    global DISPLAY_HEIGHT
    display_height = str(DISPLAY_HEIGHT)

    return display_height, 200


@app.route('/api/v1/GetDisplayModule', methods=['GET'])
def get_display_module():
    client_ip = request.remote_addr
    request_body = request.data.decode('utf-8') if request.data else ''

    logger.info(f"[HTTPServer] Received GetDisplayModule request from client {client_ip}")
    logger.debug(f"[HTTPServer] Body: {request_body}")

    # Using the global DISPLAY_MODULE variable
    global DISPLAY_MODULE
    display_module = DISPLAY_MODULE

    return display_module, 200


def set_config(key, value):
    # Placeholder function to simulate setting a configuration value
    # In a real application, implement the actual logic to save the configuration
    return True


def save_config():
    # Placeholder function to simulate saving the configuration
    # In a real application, implement the actual logic to save the configuration
    pass


if __name__ == '__main__':
    app.run(debug=True, port=5000)