#include "Translator.h"


void Translator::translate(
    const std::string& filepath,
    const std::vector<std::string>& commands,
    const Locale& input_locale,
    const Locale& output_locale)
{
    try {
        this->load_to_ptree(filepath, input_locale.code);
    }
    catch (...)
    {

    }
    
    
    //create new empty file with _translated.xml suffix
    std::string output_path = filepath.substr(0, filepath.size() - 4) + "_translated.xml";
    std::ofstream output(output_path);
    std::cout << output_path << std::endl;

    // Launch the pool with the amount of threads of local machine
    const int n_threads = std::thread::hardware_concurrency();
    boost::asio::thread_pool pool(n_threads);
   
    // translate contents according to commands
    for (const auto& cmd : commands)
    {
        //DialogsResource.Resource.Reply:text
        // slice command by two
        // parent_attr_slice[0] = DialogsResource.Resource.Reply
        // parent_attr_slice[1] = text
        std::vector<std::string> parent_attr_slice;
        boost::split(parent_attr_slice, cmd, boost::is_any_of(":"));

        //slice parent by '.'
        // parent_slice[0] = DialogsResource
        // parent_slice[1] = Resource
        // parent_slice[2] = Reply
        //etc..
        std::vector<std::string> parent_slice;
        boost::split(parent_slice, parent_attr_slice[0], boost::is_any_of("."));
        
        
        std::string parent_without_last_part;
        for (int i = 0; i < parent_slice.size() - 1; ++i)
        {
            parent_without_last_part += parent_slice[i];
            if(i != parent_slice.size() - 2)
                parent_without_last_part += ".";
        }
        try
        {
            for (auto& v : tree.get_child(parent_without_last_part))
            {
                if (v.first == parent_slice.back())
                {
                    boost::asio::post(pool, [&]() {
                        if (parent_attr_slice.size() == 1)
                        {
                            const std::string& text = v.second.data();
                            std::string response = cURL_wrapper::run_deepl_request(text, output_locale.locale_str, input_locale.locale_str);
                            v.second.put_value(response);
                        }
                        else if (parent_attr_slice.size() == 2)
                        {
                            std::string path = "<xmlattr>." + parent_attr_slice[1];
                            boost::optional<std::string> text = v.second.get_optional<std::string>(path);
                            if (text.has_value())
                            {
                                std::string response = cURL_wrapper::run_deepl_request(text.get(), output_locale.locale_str, input_locale.locale_str);
                                v.second.put(path, response);
                            }
                        }
                   });
                }   
            }
            pool.join();
        }
        catch (out_of_quota_exception& e)
        {
            pool.stop();
            this->output_to_file(output_path, output_locale.code);
            throw out_of_quota_exception();
        }
    }
    this->output_to_file(output_path, output_locale.code);
}
/// Parse the XML into the property tree.
void Translator::load_to_ptree(const std::string& filename, const std::string& locale_str)
{
    pt::read_xml(filename, tree, boost::property_tree::xml_parser::trim_whitespace ,std::locale(locale_str));   
}
/// Write property tree to XML file
void Translator::output_to_file(const std::string& filename, const std::string& locale_str)
{
    
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
