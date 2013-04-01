#include <cv.h>
#include <highgui.h>

CvCapture *video;
IplImage *image;
CvMat *thumb;
CvMat *encoded;

// Open the video file.
video = cvCaptureFromFile(filename);
if (!video) {
  // The file doesn't exist or can't be captured as a video file.
}

// Obtain the next frame from the video file
image = cvQueryFrame(video);
if (!image) {
  // Next frame doesn't exist or can't be obtained.
}

// Convert the frame to a smaller size (WIDTH x HEIGHT)
thumb = cvCreateMat(HEIGHT, WIDTH, CV_8UC3);
cvResize(img, thumb, CV_INTER_AREA);

// Encode the frame in JPEG format with JPEG quality 30%.
const static int encodeParams[] = { CV_IMWRITE_JPEG_QUALITY, 30 };
encoded = cvEncodeImage(".jpeg", thumb, encodeParams);
// After the call above, the encoded data is in encoded->data.ptr
// and has a length of encoded->cols bytes.

// Close the video file
cvReleaseCapture(&video);

