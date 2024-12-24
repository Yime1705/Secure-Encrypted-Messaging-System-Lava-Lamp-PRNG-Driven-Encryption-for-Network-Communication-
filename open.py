import cv2
import time

# Initialize the webcam
cam = cv2.VideoCapture(0)
cv2.namedWindow("Python Webcam Screenshot App")

# Counter to track the current picture index
img_counter = 0

while True:
    ret, frame = cam.read()
    if not ret:
        print("Failed to grab frame")
        break
    
    # Display the webcam feed
    cv2.imshow("Python Webcam Screenshot App", frame)
    
    k = cv2.waitKey(1)
    
    # Exit when the 'ESC' key is pressed
    if k % 256 == 27:
        print("Escape hit, closing the app")
        break
    
    # Start capturing 4 pictures when the SPACE key is pressed
    elif k % 256 == 32:
        for i in range(4):
            # Take a new frame capture for each picture
            ret, frame = cam.read()
            if not ret:
                print("Failed to grab frame")
                break
                
            img_name = f"opencv_frame_{i}.png"
            cv2.imwrite(img_name, frame)
            print(f"Screenshot saved as {img_name}")
            
            if i < 3:  # Wait only if not the last picture
                print("Waiting 5 seconds before the next picture...")
                time.sleep(5)  # Wait 5 seconds between pictures
        
        print("Captured 4 pictures.")
        break  # Exit after taking 4 pictures

# Release the webcam and close the application
cam.release()
cv2.destroyAllWindows()