#ifndef BLOB_DETECTOR_H
#define BLOB_DETECTOR_H

/// PROJECT
#include <csapex/model/node.h>

namespace csapex {
class BlobDetector : public csapex::Node
{
public:
    BlobDetector();
    ~BlobDetector();

    virtual void process();
    virtual void setup();
    virtual void setupParameters();

private:
    Output* output_;
    Output* output_debug_;
    Output* output_reduce_;

    Input* input_;
};
}

#endif // BLOB_DETECTOR_H
