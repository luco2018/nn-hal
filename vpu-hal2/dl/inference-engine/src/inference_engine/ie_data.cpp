/*
 * INTEL CONFIDENTIAL
 * Copyright 2016 Intel Corporation.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or its suppliers or licensors. Title to the Material remains with
 * Intel Corporation or its suppliers and licensors. The Material may
 * contain trade secrets and proprietary and confidential information
 * of Intel Corporation and its suppliers and licensors, and is protected
 * by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified,
 * published, uploaded, posted, transmitted, distributed, or disclosed
 * in any way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by implication,
 * inducement, estoppel or otherwise. Any license under such intellectual
 * property rights must be express and approved by Intel in writing.
 *
 * Include any supplier copyright notices as supplier requires Intel to use.
 *
 * Include supplier trademarks or logos as supplier requires Intel to use,
 * preceded by an asterisk. An asterisked footnote can be added as follows:
 * *Third Party trademarks are the property of their respective owners.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or alter
 * this notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 */

#include "ie_layers.h"
#include "ie_data.h"
#include <memory>
#include <string>
#include <map>

using namespace InferenceEngine;

Blob::Ptr Blob::CreateFromData(const DataPtr &data) {
    // TODO Here some decision should be made about the layout.
    // For now we just pass the layout and use conversion to NCHW for ANY.
    Layout targetLayout = data->getLayout();
    if (data->getLayout() == Layout::ANY) {
        targetLayout = Layout::NCHW;
    }

    switch (data->getPrecision()) {
        case Precision::FP32:
            return std::make_shared<TBlob<float>>(data->getPrecision(), targetLayout, data->getDims());
        case Precision::Q78:
        case Precision::I16:
        case Precision::FP16:
            return std::make_shared<TBlob<short>>(data->getPrecision(), targetLayout, data->getDims());
        case Precision::U8:
            return std::make_shared<TBlob<uint8_t>>(data->getPrecision(), targetLayout, data->getDims());
        default:
            THROW_IE_EXCEPTION << "precision is no set";
    }
}

Data::Data(const std::string &name, Precision _precision, Layout layout): precision(_precision), layout(layout),
                                                                          name(name), userObject({0}),
                                                                          tensorDesc(_precision, layout) {}

Data::Data(const std::string &name, const SizeVector &a_dims, Precision _precision, Layout layout)
        : precision(_precision), layout(layout), dims(a_dims), name(name), userObject({0}),
          tensorDesc(_precision, a_dims, layout) {
    SizeVector tensorDims = a_dims;
    std::reverse(tensorDims.begin(), tensorDims.end());
    tensorDesc = TensorDesc(_precision, tensorDims, layout);
}

Data::Data(const std::string &name, const TensorDesc &desc): tensorDesc(desc), precision(desc.getPrecision()),
                                                             layout(desc.getLayout()), dims(desc.getDims()),
                                                             name(name), userObject({0}) {
    std::reverse(dims.begin(), dims.end());
}

const SizeVector& Data::getDims() const {
    return tensorDesc.getDims();
}

const Precision& Data::getPrecision() const {
    if (precision)
        return precision;

    return tensorDesc.getPrecision();
}

const TensorDesc& Data::getTensorDesc() const {
    if ((tensorDesc.getDims().size() == 0 && tensorDesc.getDims() != dims) ||
            (tensorDesc.getLayout() == Layout::ANY && layout != Layout::ANY) ||
            (!tensorDesc.getPrecision() && precision)) {
        THROW_IE_EXCEPTION << "Tensor descriptor is empty!";
    }
    return tensorDesc;
}

bool Data::isInitialized() const {
    return !dims.empty() || !tensorDesc.getDims().empty();
}

void Data::setDims(const SizeVector &a_dims) {
    dims = a_dims;
    std::reverse(dims.begin(), dims.end());
    tensorDesc.setDims(a_dims);
}

void Data::setBatchSize(size_t batch_size) {
    if (dims.empty()) {
        dims = tensorDesc.getDims();
        std::reverse(dims.begin(), dims.end());
    }
    if (dims.empty())
        return;
    dims.at(dims.size() - 1) = batch_size;
    SizeVector normalDims = dims;
    std::reverse(normalDims.begin(), normalDims.end());
    tensorDesc.setDims(normalDims);
}

void Data::setLayout(Layout layout) {
    tensorDesc.setLayout(layout);
    this->layout = layout;
}

CNNLayerWeakPtr &Data::getCreatorLayer() {
    return creatorLayer;
}

const std::string &Data::getName() const {
    return name;
}

std::map<std::string, CNNLayerPtr> &Data::getInputTo() {
    return inputTo;
}

const UserValue& Data::getUserObject() const {
    return userObject;
}

Layout Data::getLayout() const {
    if (tensorDesc.getLayout() == Layout::ANY && layout != Layout::ANY)
        return layout;
    return tensorDesc.getLayout();
}

void Data::setPrecision(const Precision & precision) {
    this->precision = precision;
    tensorDesc.setPrecision(precision);
}
