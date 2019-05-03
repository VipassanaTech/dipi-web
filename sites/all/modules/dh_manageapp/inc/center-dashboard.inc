 <?php

if (isset($_REQUEST['store_center_id']) && $_REQUEST['store_center_id'] == 1) {
    $status = 0;
    $request_center = isset($_REQUEST['center_id']) ? $_REQUEST['center_id'] : '';
    if (is_array($request_center) && count($request_center) > 0) {
        unset($_SESSION['center_id']);
        $_SESSION['center_id'] = implode(',', $request_center);
        $status = 1;
    }
    echo json_encode(array('status' => $status));
    exit;
}

function get_teahcer_dashboard()
{
    global $user, $base_url;

    $center_options = array('' => 'Choose');
    $user_id = $user->uid;
    if ($user_id > 0) {
        $query = db_query("SELECT DISTINCT c.c_id as 'center_id',c.c_name as 'center_name' from dh_user_center uc INNER JOIN dh_center c on uc.uc_center=c.c_id where uc.uc_user= '" . $user_id . "' and uc.uc_deleted='0' ");
        $rows = $query->rowCount();
        if ($rows > 0) {
            if ($rows == 1) {
                $center_result = $query->fetchAssoc();
                $center_id = $center_result['center_id'];
                if ($center_id > 0) {
                    $_SESSION['multiple_center'] = '0';
                    unset($_SESSION['center_id']);
                    $_SESSION['center_id'] = $center_id;
                    $url = '/center-dashboard-page';
                    drupal_goto($url);
                }
            } else if ($rows >= 2) {
                while ($result = $query->fetchAssoc()) {
                    $center_options[$result['center_id']] = $result['center_name'];
                }

                if (is_array($center_options) && count($center_options) > 0) {
                    $_SESSION['multiple_center'] = '1';
                    $wt = 1;
                    $buttons = "<table style='border-collapse: separate;border-spacing:20px 10px;'>";
                    $i = 1;
                    foreach ($center_options as $key => $val) {
                        if ($key != "" && !empty($key)) {

                            if ($i == 1) {
                                $buttons .= "<tr>";
                            }

                            $buttons .= '<td><button class="btn btn_clk btn-block"   value="' . $key . '"  id="user_center_' . $key . '">' . $val . '</button></td>';

                            if ($i == 4) {
                                $i = 1;
                                $buttons .= "</tr>";
                            } else {
                                $i++;
                            }
                        }
                    }

                    $buttons .= '</table>';

                    $group = 'Select centers to see dashboard';

                    $form[$group] = array('#type' => 'fieldset', '#title' => $group);

                    $form[$group]['center_buttons'] = array(
                        '#title' => t(''),
                        '#type' => 'item',
                        '#weight' => $wt++,
                        '#name' => 'center_buttons',
                        '#id' => 'center_buttons',
                        '#markup' =>  $buttons,
                    );

                    $form['']['submit'] = array(
                        '#type' => 'button',
                        '#value' => 'Submit',
                        '#weight' => $wt++,
                        '#name' => 'Submit',
                        '#id' => 'Submit',
                        '#attributes' => array("onclick" => "show_teacher_dashboard();return false;"),
                        '#suffix' => '</div>',

                    );

                    $js =
                        '
                                jQuery(document).ready(function(){
                                                    jQuery(".btn_clk") . click(function () {
                                                            if (jQuery(this) . hasClass("btn-info")) {
                                                                    jQuery(this) . removeClass("btn-info");
                                                                    return false;
                                                            }else{
                                                                jQuery(this) . addClass("btn-info");
                                                                return false;
                                                            }
                                                    });
                                });


                                function show_teacher_dashboard()
								{
                                    var selected_center=[];
                                        jQuery(".btn_clk").each(function(){
                                                if(jQuery(this).hasClass("btn-info")){
                                                                 selected_center.push(jQuery(this).attr("value"));
                                                }
                                        });

                                    var center_len= selected_center.length;
                                    if(center_len<1){
                                        alert("Please select atleast one center....");
                                        return false;
                                    }else{
                                         var center_id = selected_center;
									    if(center_id !=""){
                                                        jQuery.ajax({
                                                             url:"",
                                                             data:{"store_center_id":1,"center_id":center_id},
                                                             dataType:"json",
                                                             type:"POST",
                                                             success : function(response){
                                                                    var status=  response.status;
                                                                    if(status==1){
                                                                        location.href="/center-dashboard-page";
                                                                    }else{
                                                                        alert("Please select atleast one center....");
                                                                    }
                                                             }
                                                        });
                                                return false;
									    }
                                    }

                                    function change_btn_color(centerid)
                                    {
                                        console.log(centerid);
                                        return false;
                                    }

								}';
                    drupal_add_js($js, 'inline');

                    return $form;
                }
            }
        } else {
            return "<div style='text-align:center;'><h2>There is no center id mapped with your id..</h2></div>";
        }
    }
}

function get_user_logout()
{
    global $user;
    global $base_url;
    watchdog('user', 'Session closed for %name.', array('%name' => $user->name));
    module_invoke_all('user_logout', $user);
    session_destroy();
    drupal_goto($base_url);
}
