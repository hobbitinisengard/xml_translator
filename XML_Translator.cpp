#include "XML_Translator.h"


bool Xml_translator::translate(
    const std::string& filepath,
    const std::vector<std::string>& commands,
    const Locale& input_locale,
    const Locale& output_locale)
{
    this->load_to_ptree(filepath, input_locale.code);
    
    //create new empty file with _translated.xml suffix
    std::string output_path = filepath.substr(0, filepath.size() - 4) + "_translated.xml";
    std::ofstream output(output_path);
    std::cout << output_path << std::endl;

    // Launch the pool with n threads.
    const int n_threads = std::thread::hardware_concurrency();
    boost::asio::thread_pool pool(n_threads);
   
    // translate contents according to commands
    for (const auto& cmd : commands)
    {
        //DialogsResource.Resource.Reply:text
        std::vector<std::string> parent_or_attr;
        boost::split(parent_or_attr, cmd, boost::is_any_of(":"));

        // parent_or_attr[0] = DialogsResource.Resource.Reply
        // parent_or_attr[1] = text
        
        std::vector<std::string> proper_parent;
        boost::split(proper_parent, parent_or_attr[0], boost::is_any_of("."));
        // proper_parent[0] = DialogsResource
        // proper_parent[1] = Resource
        // proper_parent[2] = Reply
        
        std::string joined_proper_parent;
        for (int i = 0; i < proper_parent.size() - 1; ++i)
        {
            joined_proper_parent += proper_parent[i];
            if(i != proper_parent.size() - 2)
                joined_proper_parent += ".";
        }
        try
        {
            for (auto& v : tree.get_child(joined_proper_parent))
            {
                if (v.first == proper_parent.back())
                {
                    boost::asio::post(pool, [&]() {
                        if (parent_or_attr.size() == 1)
                        {
                            const std::string& text = v.second.data();
                            std::string response = cURL_wrapper::run_request(text, output_locale.locale_str, input_locale.locale_str);
                            v.second.put_value(response);
                        }
                        else if (parent_or_attr.size() == 2)
                        {
                            std::string path = "<xmlattr>." + parent_or_attr[1];
                            boost::optional<std::string> text = v.second.get_optional<std::string>(path);
                            if (text.has_value())
                            {
                                std::string response = cURL_wrapper::run_request(text.get(), output_locale.locale_str, input_locale.locale_str);
                                v.second.put(path, response);
                            }
                        }
                   });
                }   
            }
            pool.join();
        }
        catch (...)
        {
            this->output_to_file(output_path, output_locale.code);
            return false;
        }
    }
    this->output_to_file(output_path, output_locale.code);
    return true;
}
void Xml_translator::load_to_ptree(const std::string& filename, const std::string& locale_str)
{
    // Parse the XML into the property tree.
    pt::read_xml(filename, tree, boost::property_tree::xml_parser::trim_whitespace ,std::locale(locale_str));   
}
void Xml_translator::output_to_file(const std::string& filename, const std::string& locale_str)
{
    // Write property tree to XML file
    pt::xml_writer_settings<std::string> settings('\t', 1);
    try
    {
        pt::write_xml(filename, tree, std::locale(locale_str), settings);
    }
    catch (pt::xml_parser_error& e)
    {
        std::cout << e.what() << std::endl;
    }
}
