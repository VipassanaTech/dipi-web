(function ($) {

	function get_age( datestr ) 
	{
	    var today = new Date();
	    var birthDate = new Date( datestr );
	    var age = today.getFullYear() - birthDate.getFullYear();
	    var m = today.getMonth() - birthDate.getMonth();
	    if (m < 0 || (m === 0 && today.getDate() < birthDate.getDate())) {
	        age--;
	    }
	    return age;
	}

	function check_mobile()
	{
		var val = $("#edit-a-phone-mobile").val();
		if ( ($("#edit-a-country").val() == "IN") && (val.length > 0)  )
		{
			if (/^\d{10}$/.test(val)) {
			    // value is ok, use it
			    return true;
			} else {
			    alert("Invalid mobile number; must be ten digits")
			    $("#edit-a-phone-mobile").focus()
			    return false;
			}			
		}
		return true;
	}


	function check_oldstudent()
	{
		if($("input[name=a_old]:checked").val() == "1")
			$("#edit-course-information :input").prop("disabled", false);
		else
			$("#edit-course-information :input").prop("disabled", true);				

	}
	function check_attending()
	{
		//alert($("input[name=attending]:checked").val());
		if($("input[name=attending]:checked").val() == "1")
			$("#attending-fields :input").prop("disabled", false);
		else
			$("#attending-fields :input").prop("disabled", true);				
	}

  function check_left()
  {
    if($("input[name=aa_left]:checked").val() == "1")
      $("#left-fields :input").prop("disabled", false);
    else
      $("#left-fields :input").prop("disabled", true);
  }

	function check_acco()
	{
		//alert("Acco - "+$("input[name=acco_old]").val());
		if ($("input[name=acco_old]").val() != "")
		{
			$("#room-section").hide();
		}
	}

	function check_relationship()
	{
		if($("input[name=al_relationship]:checked").val() == "1")
		{
			$(".relationship :input").attr("disabled", false);
		}	
		else
			$(".relationship :input").attr("disabled", true);

	}

	function check_teacher()
	{
		if($("input[name=ac_teacher]:checked").val() == "1")
			$("#edit-ac-teacher-code").prop("disabled", false);
		else
		{
			$("#edit-ac-teacher-code").prop("disabled", true);				
			$("#edit-ac-teacher-code").val("");
		}
	}


	$(document).ready(function(){
		//alert("hi");
		$(document).ajaxComplete(function () {
			//alert("This works");
  			check_attending();
        check_left();
		  	check_oldstudent();
		  	check_acco();
		  	check_relationship();
		  	check_teacher();
//		  	$("#edit-a-zip").focusout(function(){ 
//		  		alert("hi");
//		  		do_pincode($(this).val() ); 
//		  	});
		});

		var checkExist = setInterval(function() {
		   if ($("input[name=attending]") && $("input[name=a_old]") && $("input[name=a_gender]")) 
		   {
	  		  check_attending();
			  check_oldstudent();
	 	  	  check_teacher();
			  $("input[name=a_gender]").trigger("change");
			  check_acco();
		      clearInterval(checkExist);
		   }
		}, 1000);


		$(document).on("change", "input[type=radio][name=attending]", function(){
			check_attending();
		});

    $(document).on("change", "input[type=radio][name=aa_left]", function(){
      check_left();
    });

		$(document).on("change", "input[type=radio][name=a_old]", function(){
			check_oldstudent();
		});

		$(document).on("change", "input[type=radio][name=al_relationship]", function(){
			check_relationship();
		});

		$(document).on("change", "input[type=radio][name=ac_teacher]", function(){
			check_teacher();
		});


	  	$("#edit-a-zip").focusout(function(){ 
//		  		alert("hi");
	  		do_pincode($(this).val(), "#edit-a-country", "#edit-a-state", "#edit-a-city" ); 
	  	});

		$("#edit-age").focusout(function(){
			//alert("hi");
			var year = new Date().getFullYear();
			var a = year - $(this).val();
			//console.log(a + $("#edit-a-dob-datepicker-popup-0").val());
			if ( $("#edit-a-dob-datepicker-popup-0").val() == "" )
				$("#edit-a-dob-datepicker-popup-0").val(a+"-01-01"); 
			if ( $("#edit-s-dob-datepicker-popup-0").val() == "" )
				$("#edit-s-dob-datepicker-popup-0").val(a+"-01-01"); 

		})		 
		
		$(document).on("click", "#change-room", function(){
			$("#room-section").show();
			return false;
		});

		autocomplete_country("#edit-a-country");
		autocomplete_state("#edit-a-state", "#edit-a-country");
		autocomplete_city("#edit-a-city", "#edit-a-country", "#edit-a-state");


		window.autocomplete_teacher = function ( element_teacher, teacher_type = 'all', value_or_code = 'code' )
		{
			$(document).on("keydown.autocomplete", element_teacher, function(){
				$(this).autocomplete({
					source: function( request, response ) {
						$(element_teacher).addClass( "throbbing" );
				        $.ajax({
				          url: "/autocomplete/get-teacher/" + request.term + "?type=" + teacher_type,
				          dataType: "json",
				          success: function( data ) {
				          	$(element_teacher).removeClass( "throbbing" );
				            response( data );
				          }
				        });
					},
		  			minLength: 1,
		  			select: function( event, ui ) {
		        		//console.log( "Selected: " + ui.item.value + " aka " + ui.item.code );
		        		if ( value_or_code == 'code' )
		        			$( element_teacher ).val( ui.item.code );
		        		else
		        			$( element_teacher ).val( ui.item.fullname );
		        		return false;
		      		}
	      		});
			});
		}

		autocomplete_teacher( "#edit-al-recommending", 'all', 'value' );
		// autocomplete_teacher( "#edit-al-area-at", 'full-t', 'value' );
    $("#edit-al-area-at").select2();

		$("#edit-ac-teacher-code").autocomplete({
			source: function( request, response ) {
				$("#edit-ac-teacher-code").addClass( "throbbing" );
		        $.ajax({
		          url: "/autocomplete/get-teacher/" + request.term,
		          dataType: "json",
		          success: function( data ) {
		          	$("#edit-ac-teacher-code").removeClass( "throbbing" );
		            response( data );
		          }
		        });
			},
  			minLength: 1,
  			select: function( event, ui ) {
        		//console.log( "Selected: " + ui.item.value + " aka " + ui.item.code );
        		$( "#edit-ac-teacher-code" ).val( ui.item.code );
        		return false;
      		}
  		});


		$("#dh-ma-applicant-form").submit(function(event){
			/*if( ! $("#edit-checked-address").is(':checked'))
			{
				alert("Please check Address");
				$("#edit-a-address").focus();
				event.preventDefault();
				return false;

			}
			if( ! $("#edit-checked-dob").is(':checked'))
			{
				alert("Please check Date of Birth");
				$("#edit-a-dob-datepicker-popup-0").focus();
				event.preventDefault();
				return false;
			}*/
			if (! check_mobile() )
			{
				event.preventDefault();				
				return false;
			}

			if ($("#edit-a-dob-datepicker-popup-0").val() == "")
			{
				alert("Please enter Date of Birth");
				$("#edit-a-dob-datepicker-popup-0").focus();
				event.preventDefault();
				return false;				
			}
			var age = get_age($("#edit-a-dob-datepicker-popup-0").val());
			if ( (age > 95) || (age < 9 ) )
			{
				alert("Please check Date of Birth");
				$("#edit-a-dob-datepicker-popup-0").focus();
				event.preventDefault();
				return false;					
			}

			if($("input[name=a_old]:checked").val() == "1")
			{
				var t = parseInt($("#edit-ac-10d").val()) || 0;
				var t1 = parseInt($("#edit-ac-teen").val()) || 0;
				if ( (t <= 0) && (t1 <=0) )
				{
					alert("Old Student must have attened at least one Teenager/10 day course");
					$("#edit-ac-teen").focus();
					event.preventDefault();
					return false;
				}
				if($("input[name=ac_teacher]:checked").val() == "1")
				{
					if ($("#edit-ac-teacher-code").val() == "")
					{
						alert("Please put AT Code");
						$("#edit-ac-teacher-code").focus();
						event.preventDefault();
						return false;						
					}
				}
			}

			if($("input[name=aa_left]:checked").val() == "1")
			{
				if ($("#edit-a-dob-datepicker-popup-1").val() == "")
				{
					alert("Please enter Date/Time that the student left");
					$("#edit-a-dob-datepicker-popup-1").focus();
					event.preventDefault();
					return false;				
				}
			}

			if ( $("#edit-a-f-name").val().trim().length == 0 )
			{
				alert("First Name is required");
				$("#edit-a-f-name").focus();
				event.preventDefault();
				return false;
			}

			if ( $("#edit-a-l-name").val().trim().length == 0 )
			{
				alert("Last Name is required");
				$("#edit-a-l-name").focus();
				event.preventDefault();
				return false;
			}

			if ( $("#edit-a-address").val().trim().length == 0 )
			{
				alert("Address is required");
				$("#edit-a-address").focus();
				event.preventDefault();
				return false;
			}

			if ( ($("#edit-a-country").val().length > 2) || ($("#edit-a-country").val().trim().length == 0)  )
			{
				alert("Country should be 2 character ISO code");
				$("#edit-a-country").focus();
				event.preventDefault();
				return false;
			}

			if ( ($("#edit-a-state").val().length > 3) || ($("#edit-a-state").val().trim().length == 0 ) )
			{
				alert("State should be ISO code");
				$("#edit-a-state").focus();
				event.preventDefault();
				return false;
			}

			if ( $("#edit-a-source").val().trim().length == 0 )
			{
				alert("Please select Application Mode");
				$("#edit-a-source").focus();
				event.preventDefault();
				return false;
			}

			return true;
		});

	});
})(jQuery);
