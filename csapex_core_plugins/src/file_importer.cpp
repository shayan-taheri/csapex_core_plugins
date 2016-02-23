/// HEADER
#include "file_importer.h"

/// PROJECT
#include <csapex/manager/message_provider_manager.h>
#include <csapex/model/node_modifier.h>
#include <csapex/msg/io.h>
#include <csapex/msg/message.h>
#include <csapex/signal/trigger.h>
#include <csapex/utility/register_apex_plugin.h>
#include <csapex/utility/timer.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/param/range_parameter.h>
#include <csapex/msg/any_message.h>
#include <csapex/utility/interlude.hpp>
#include <csapex/signal/slot.h>

/// SYSTEM
#include <boost/filesystem.hpp>
#include <boost/lambda/lambda.hpp>
#include <QDir>
#include <QUrl>

CSAPEX_REGISTER_CLASS(csapex::FileImporter, csapex::Node)


using namespace csapex;
using namespace connection_types;


FileImporter::FileImporter()
    : playing_(false), end_triggered_(false),
      directory_import_(false), last_directory_index_(-1), cache_enabled_(false)
{
}

FileImporter::~FileImporter()
{
}


void FileImporter::setupParameters(Parameterizable& parameters)
{
    csapex::param::Parameter::Ptr directory = csapex::param::ParameterFactory::declareBool("import directory", false);
    parameters.addParameter(directory);

    std::function<bool()> cond_file = [directory]() { return !directory->as<bool>(); };
    std::function<bool()> cond_dir = [directory]() { return directory->as<bool>(); };

    parameters.addConditionalParameter(csapex::param::ParameterFactory::declareBool("recursive import", false), cond_dir);

    std::string filter = std::string("Supported files (") + MessageProviderManager::instance().supportedTypes() + ");;All files (*.*)";
    parameters.addConditionalParameter(csapex::param::ParameterFactory::declareFileInputPath("path", "", filter), cond_file, std::bind(&FileImporter::import, this));

    parameters.addConditionalParameter(csapex::param::ParameterFactory::declareDirectoryInputPath("directory", ""), cond_dir, std::bind(&FileImporter::import, this));
    parameters.addConditionalParameter(csapex::param::ParameterFactory::declareRange<int>("directory/current", 0, 1, 0, 1), cond_dir, std::bind(&FileImporter::changeDirIndex, this));

    parameters.addConditionalParameter(csapex::param::ParameterFactory::declareBool("directory/play", true), cond_dir);
    parameters.addConditionalParameter(csapex::param::ParameterFactory::declareBool("directory/loop", true), cond_dir);
    parameters.addConditionalParameter(csapex::param::ParameterFactory::declareBool("directory/latch", false), cond_dir);

    csapex::param::Parameter::Ptr immediate = csapex::param::ParameterFactory::declareBool("playback/immediate", false);
    parameters.addParameter(immediate, std::bind(&FileImporter::changeMode, this));

    std::function<void(csapex::param::Parameter*)> setf = std::bind(&TickableNode::setTickFrequency, this, std::bind(&csapex::param::Parameter::as<double>, std::placeholders::_1));
    std::function<bool()> conditionf = [immediate]() { return !immediate->as<bool>(); };
    addConditionalParameter(csapex::param::ParameterFactory::declareRange("playback/frequency", 1.0, 256.0, 30.0, 0.5), conditionf, setf);

    parameters.addParameter(csapex::param::ParameterFactory::declareBool("cache", 0), [this](param::Parameter* p) {
        cache_enabled_ = p->as<bool>();
        if(!cache_enabled_) {
            cache_.clear();
        }
    });

}

void FileImporter::setup(NodeModifier& node_modifier)
{
    outputs_.push_back(node_modifier.addOutput<connection_types::AnyMessage>("Unknown"));

    begin_ = node_modifier.addTrigger("begin");
    end_ = node_modifier.addTrigger("end");

    node_modifier.addSlot("restart", [this](){
        ainfo << "restart" << std::endl;
        playing_ = true;
        if(directory_import_) {
            setParameter("directory/current", 0);
            int current = readParameter<int>("directory/current");
            ainfo << "set current to "  << current << std::endl;
        } else {
            if(provider_) {
                provider_->restart();
            }
        }
    });
    play_ = node_modifier.addSlot("play", [this](){
        setParameter("directory/play", true);
        playing_ = true;
    });
    node_modifier.addSlot("stop", [this](){
        setParameter("directory/current", -1);
        setParameter("directory/play", false);
        playing_ = false;
    });
    node_modifier.addSlot("abort", [this](){
        setParameter("directory/play", false);
        setParameter("directory/latch", false);
        ainfo << "abort" << std::endl;
        setParameter("directory/current", (int) dir_files_.size());
        playing_ = false;
        end_->trigger();
    });
    play_->connected.connect([this](){
        setParameter("directory/play", false);
        playing_ = false;
    });
}

void FileImporter::changeMode()
{
    if(readParameter<bool>("playback/immediate")) {
        setTickFrequency(-1.0);
    } else {
        setTickFrequency(readParameter<double>("playback/frequency"));
    }
}

void FileImporter::process(csapex::NodeModifier& node_modifier, csapex::Parameterizable& parameters)
{
    tick(node_modifier, parameters);
}

bool FileImporter::canTick()
{
    if(!node_modifier_->isSource()) {
        return false;
    }
    if(directory_import_) {        
        if(play_->isConnected()) {
            bool can_tick = playing_ && readParameter<int>("directory/current") <= (int) dir_files_.size();
            if(!can_tick) {
           //     ainfo << "cannot tick: " << playing_ << ", " << readParameter<int>("directory/current") << " <= " <<  (int) dir_files_.size()<< std::endl;
            }
            return can_tick;
        }
        return (readParameter<int>("directory/current") < (int) dir_files_.size()) ||
                readParameter<bool>("directory/loop")  ||
                readParameter<bool>("directory/latch");
    } else {
        if(play_->isConnected()) {
            if(!playing_) {
                return false;
            }
        }
        return provider_ != nullptr;
    }
}

bool FileImporter::tick(csapex::NodeModifier& nm, csapex::Parameterizable& p)
{
    if(provider_ && provider_->hasNext()) {
        for(std::size_t slot = 0, total = provider_->slotCount(); slot < total; ++slot) {
            INTERLUDE_STREAM("slot " << slot);

            Output* output = outputs_[slot];

            if(msg::isConnected(output)) {
                Message::Ptr msg = provider_->next(slot);
                if(msg) {
                    msg::publish(output, msg);
                }
            }
        }
        return true;

    } else if(directory_import_) {
        bool play = readParameter<bool>("directory/play") ||
                (play_->isConnected() && playing_);
        bool latch = readParameter<bool>("directory/latch");
        bool loop = readParameter<bool>("directory/loop");

        int current = readParameter<int>("directory/current");
        bool index_changed = current != last_directory_index_;

        if(!play && !latch && !index_changed) {
            return false;
        }
        INTERLUDE("directory");

        int files = dir_files_.size();

        ainfo << "current: " << current << std::endl;
        if(current >= files) {
            if(!end_triggered_) {
                end_->trigger();
                ainfo << "end" << std::endl;
            }

            if((play_->isConnected() && playing_)) {
                //playing_ = false; // it can happen that playing_ is set to true before we do this...
                end_triggered_ = true;
                return false;
            }

            if(loop) {
                current = 0;
            } else if(latch && files > 0) {
                current = files - 1;
            } else {
                setParameter("directory/play", false);
            }
        }

        if(current < 0) {
            current = 0;
        }

        if(current < files) {
            if(current == 0) {
                end_triggered_ = false;
                begin_->trigger();
            }

            createMessageProvider(QString::fromStdString(dir_files_.at(current)));


            if(play) {
                ++current;
            }

            ainfo << "set current " << current << std::endl;
            setParameter("directory/current", current);
            int current = readParameter<int>("directory/current");
            ainfo << "have set current to "  << current << std::endl;
            last_directory_index_ = current;
        }
    }


    return false;
}

void FileImporter::doImportDir(const QString &dir_string)
{

    dir_files_.clear();

    bool recursive = readParameter<bool>("recursive import");

    std::function<void(const boost::filesystem::path&)> crawl_dir = [&](const boost::filesystem::path& directory) {
        boost::filesystem::directory_iterator dir(directory);
        boost::filesystem::directory_iterator end;
        for(; dir != end; ++dir) {
            boost::filesystem::path path = dir->path();

            if(boost::filesystem::is_directory(path)) {
                if(recursive) {
                    crawl_dir(path);
                }
            } else {
                dir_files_.push_back(path.string());
            }

        }
    };
    boost::filesystem::path directory(dir_string.toStdString());
    crawl_dir(directory);

    std::sort(dir_files_.begin(), dir_files_.end());

    param::RangeParameter::Ptr current = getParameter<param::RangeParameter>("directory/current");
    current->set(0);
    current->setMax<int>(dir_files_.size());
}

bool FileImporter::createMessageProvider(const QString& file_path)
{
    INTERLUDE("doImport");
    if(file_path.isEmpty()) {
        node_modifier_->setWarning("no file selected");
        return false;
    }
    bool latch = readParameter<bool>("directory/latch");
    if((file_path == file_ && !latch) && provider_) {
        return false;
    }

    QString path;
    QFile file(path);
    if(file.exists()) {
        path = file_path;
    } else {
        QUrl url(file_path);
        QFile urlfile(file_path);

        if(urlfile.exists()) {
            path = urlfile.fileName();
        } else {
            node_modifier_->setError(std::string("the file ") + file_path.toStdString() + " couldn't be opened");
            return false;
        }
    }

    file_ = file_path;
    node_modifier_->setNoError();

    try {
        std::string path_str = path.toStdString();
        auto pos = cache_.find(path_str);
        if(pos != cache_.end()) {
            provider_ = pos->second;
        } else {

            if(!directory_import_) {
                removeTemporaryParameters();
            }

            INTERLUDE("createMessageProvider");
            provider_ = MessageProviderManager::createMessageProvider(path.toStdString());
            if(cache_enabled_) {
                cache_[path_str] = provider_;
            }
        }

        provider_->slot_count_changed.connect(std::bind(&FileImporter::updateOutputs, this));

        if(!directory_import_) {
            provider_->begin.connect(std::bind(&Trigger::trigger, begin_));
            provider_->no_more_messages.connect(std::bind(&Trigger::trigger, end_));
        }

        {
            INTERLUDE("load");
            provider_->load(path.toStdString());
        }

        if(!directory_import_) {
            setTemporaryParameters(provider_->getParameters(), std::bind(&FileImporter::updateProvider, this));
        }

        return provider_.get();

    } catch(const std::exception& e) {
        node_modifier_->setNoError();
        throw std::runtime_error(std::string("cannot load file ") + file_.toStdString() + ": " + e.what());
    }

    return false;
}

void FileImporter::updateProvider()
{
    if(provider_) {
        provider_->parameterChanged();
    }
}

void FileImporter::updateOutputs()
{
    std::size_t slot_count = provider_->slotCount();

    std::size_t output_count = outputs_.size();

    if(slot_count > output_count) {
        for(std::size_t i = output_count ; i < slot_count ; ++i) {
            outputs_.push_back(node_modifier_->addOutput<AnyMessage>("unknown"));
        }
    } else {
        bool del = true;
        for(int i = output_count-1 ; i >= (int) slot_count; --i) {
            Output* output = outputs_[i];
            if(msg::isConnected(output)) {
                del = false;
            }

            if(del) {
                node_modifier_->removeOutput(msg::getUUID(output));
                outputs_.erase(outputs_.begin() + i);
            } else {
                msg::disable(output);
            }
        }
    }

    for(std::size_t i = 0; i < slot_count; ++i) {
        Output* out = outputs_[i];
        msg::setLabel(out, provider_->getLabel(i));
    }
}

void FileImporter::changeDirIndex()
{
    //    int set_to = readParameter<int>("directory/current");
    //    setParameter("directory/current", set_to);
}

void FileImporter::import()
{
    directory_import_ = readParameter<bool>("import directory");
    provider_.reset();

    if(directory_import_) {
        doImportDir(QString::fromStdString(readParameter<std::string>("directory")));
        removeTemporaryParameters();
    } else {
        createMessageProvider(QString::fromStdString(readParameter<std::string>("path")));
    }
}
