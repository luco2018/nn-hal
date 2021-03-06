//
// INTEL CONFIDENTIAL
// Copyright 2016 Intel Corporation.
//
// The source code contained or described herein and all documents
// related to the source code ("Material") are owned by Intel Corporation
// or its suppliers or licensors. Title to the Material remains with
// Intel Corporation or its suppliers and licensors. The Material may
// contain trade secrets and proprietary and confidential information
// of Intel Corporation and its suppliers and licensors, and is protected
// by worldwide copyright and trade secret laws and treaty provisions.
// No part of the Material may be used, copied, reproduced, modified,
// published, uploaded, posted, transmitted, distributed, or disclosed
// in any way without Intel's prior express written permission.
//
// No license under any patent, copyright, trade secret or other
// intellectual property right is granted to or conferred upon you by
// disclosure or delivery of the Materials, either expressly, by implication,
// inducement, estoppel or otherwise. Any license under such intellectual
// property rights must be express and approved by Intel in writing.
//
// Include any supplier copyright notices as supplier requires Intel to use.
//
// Include supplier trademarks or logos as supplier requires Intel to use,
// preceded by an asterisk. An asterisked footnote can be added as follows:
// *Third Party trademarks are the property of their respective owners.
//
// Unless otherwise agreed by Intel in writing, you may not remove or alter
// this notice or any other notice embedded in Materials by Intel or Intel's
// suppliers or licensors in any way.
//
#pragma once

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <cpp_interfaces/impl/ie_executable_network_thread_safe_default.hpp>

#include "mkldnn_memory.h"
#include "config.h"
#include "perf_count.h"
#include "mkldnn_dims.h"
#include "mean_image.h"
#include "mkldnn_node.h"
#include "mkldnn_edge.h"
#include "mkldnn_extension_utils.h"

namespace MKLDNNPlugin {

class MKLDNNGraph {
public:
    typedef std::shared_ptr<MKLDNNGraph> Ptr;

    enum Status {
        NotReady = 0,
        Ready = 1,
    };

    MKLDNNGraph(): status(NotReady), eng(mkldnn::engine(mkldnn::engine::kind::cpu, 0)) {}

    Status GetStatus() {
        return status;
    }

    bool IsReady() {
        return (GetStatus() == Ready);
    }

    void setConfig(const Config &cfg);
    void setProperty(const std::map<std::string, std::string> &properties);
    Config getProperty();

    void getInputBlobs(InferenceEngine::BlobMap &in_map);
    void getOutputBlobs(InferenceEngine::BlobMap &out_map);

    void CreateGraph(InferenceEngine::ICNNNetwork &network, const MKLDNNExtensionManager::Ptr& extMgr);

    bool hasMeanImageFor(const std::string& name) {
        return _meanImages.find(name) != _meanImages.end();
    }

    void PushInputData(const std::string& name, const InferenceEngine::Blob::Ptr &in);
    void PullOutputData(InferenceEngine::BlobMap &out);

    void Infer();

    std::vector<MKLDNNNodePtr>& GetNodes() {
        return graphNodes;
    }

    std::vector<MKLDNNEdgePtr>& GetEdges() {
        return graphEdges;
    }

    std::vector<MKLDNNNodePtr>& GetOutputNodes() {
        return outputNodes;
    }

    mkldnn::engine getEngine() const {
        return eng;
    }

    void GetPerfData(std::map<std::string, InferenceEngine::InferenceEngineProfileInfo> &perfMap) const;

protected:
    MKLDNNNodePtr ParseNode(const InferenceEngine::CNNLayerPtr& cnnLayer, MKLDNNNodePtr& parent,
                            const MKLDNNExtensionManager::Ptr& extMgr, size_t outIdx);

    MKLDNNNodePtr FindNodeWithName(const std::string& name) const;
    void VisitNode(MKLDNNNodePtr node, std::vector<MKLDNNNodePtr>& sortedNodes);
    void SortTopologically();

    void ForgetGraphData() {
        status = NotReady;
        eng = mkldnn::engine(mkldnn::engine::kind::cpu, 0);

        inputNodes.clear();
        outputNodes.clear();
        graphNodes.clear();
        graphEdges.clear();
        _meanImages.clear();
    }
    Status status;
    Config config;

    std::map<std::string, MKLDNNNodePtr> inputNodes;
    std::vector<MKLDNNNodePtr> outputNodes;
    std::vector<MKLDNNNodePtr> graphNodes;
    std::vector<MKLDNNEdgePtr> graphEdges;

    std::map<std::string, MeanImage> _meanImages;

    mkldnn::engine eng;

    void InitNodes();
    void SelectOptimalPrimitiveDescriptors();
    void InitEdges();
    void Allocate();
    void CreatePrimitives();

    friend class MKLDNNInferRequest;
};


class MKLDNNExecNetwork: public InferenceEngine::ExecutableNetworkThreadSafeDefault {
public:
    typedef std::shared_ptr<MKLDNNExecNetwork> Ptr;

    InferenceEngine::InferRequestInternal::Ptr CreateInferRequestImpl(InferenceEngine::InputsDataMap networkInputs,
                                                                      InferenceEngine::OutputsDataMap networkOutputs) override;

    void CreateInferRequest(InferenceEngine::IInferRequest::Ptr &asyncRequest) override;

    MKLDNNExecNetwork(InferenceEngine::ICNNNetwork &network, const Config &cfg,
                      const MKLDNNExtensionManager::Ptr& extMgr);

    ~MKLDNNExecNetwork() override;

    void setProperty(const std::map<std::string, std::string> &properties);

protected:
    MKLDNNGraph::Ptr graph;
    MKLDNNExtensionManager::Ptr extensionManager;
};

}  // namespace MKLDNNPlugin
